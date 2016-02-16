/*
 * TTY driver
 */

#include "kernel/stalloc.h"
#include "kernel/spinlock.h"
#include "kernel/sleepq.h"
#include "kernel/interrupt.h"
#include "kernel/thread.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "drivers/drivers.h"
#include "drivers/device.h"
#include "drivers/gcd.h"
#include <tty.h>

/**@name TTY driver
 *
 * This module contains functions for interrupt driven TTY driver.
 *
 * @{
 */

static int tty_write(gcd_t *gcd, const void *buf, int len);
static int tty_read(gcd_t *gcd, void *buf, int len);

/* We need this spinlock so that we can synchronise with the polling
 * tty drivers writes, since this driver cannot be used in some parts
 * of the kernel.
 */
extern spinlock_t kprintf_slock;

/**
 * Initializes interrupt driven tty driver. Memory is reserved for
 * data structures and tty interrupt handler is registerded.
 *
 * @param desc Pointer to a YAMS device descriptor data structure.
 *
 * @return Pointer to tty's device_t structure.
 */
device_t *tty_init(io_descriptor_t *desc) {
  device_t *dev;
  gcd_t *gcd;
  tty_real_device_t *tty_rd;
  uint32_t irq_mask;
  static int num_of_inits = 0;

  dev = (device_t*)stalloc(sizeof(device_t));
  if(dev == NULL)
    KERNEL_PANIC("Could not reserve memory for tty driver.");

  gcd = (gcd_t*)stalloc(sizeof(gcd_t));
  if(gcd == NULL)
    KERNEL_PANIC("Could not reserve memory for tty driver.");

  dev->generic_device = gcd;
  dev->io_address     = desc->io_area_base;
  dev->type           = desc->type;

  gcd->device = dev;
  gcd->write  = tty_write;
  gcd->read   = tty_read;

  tty_rd = (tty_real_device_t*)stalloc(sizeof(tty_real_device_t));
  if(tty_rd == NULL)
    KERNEL_PANIC("Could not reserve memory for tty driver.");

  dev->real_device = tty_rd;
  if (num_of_inits == 0) {
    /* First tty driver will share the device with the polling TTY.
     * That is, we use the same spinlock with it. (The spinlock is
     * kprintf's because that is the only proper way to access the
     * polling tty.) */
    tty_rd->slock = &kprintf_slock;
  } else {
    tty_rd->slock = (spinlock_t*)stalloc(sizeof(spinlock_t));
    if(tty_rd->slock == NULL)
      KERNEL_PANIC("Could not reserve memory for tty driver spinlock.");
    spinlock_reset(tty_rd->slock);
  }
  num_of_inits++;

  tty_rd->write_head = 0;
  tty_rd->write_count = 0;

  tty_rd->read_head = 0;
  tty_rd->read_count = 0;

  irq_mask = 1 << (desc->irq + 10);
  interrupt_register(irq_mask, tty_interrupt_handle, dev);

  return dev;
}

/**
 * TTY's interrupt handler. Functinality depends on status of TTY's
 * status port. On WIRQ status writes internal buffer from
 * tty_real_device_t data structure to data port. On RIRQ
 * status reads data from data port to the internal buffer.
 * Implements read from the gbd interface.
 *
 * @param device Pointer to the TTY device.
 */
void tty_interrupt_handle(device_t *device) {
  volatile tty_io_area_t *iobase = (tty_io_area_t *)device->io_address;
  volatile tty_real_device_t *tty_rd
    = (tty_real_device_t *)device->real_device;

  if(TTY_STATUS_WIRQ(iobase->status)) {
    spinlock_acquire(tty_rd->slock);

    iobase->command = TTY_COMMAND_WIRQD;
    iobase->command = TTY_COMMAND_WIRQ;
    while(!TTY_STATUS_WBUSY(iobase->status) && tty_rd->write_count > 0) {
      iobase->command = TTY_COMMAND_WIRQ;
      iobase->data = tty_rd->write_buf[tty_rd->write_head];
      tty_rd->write_head = (tty_rd->write_head + 1) % TTY_BUF_SIZE;
      tty_rd->write_count--;
    }
    iobase->command = TTY_COMMAND_WIRQE;

    if (tty_rd->write_count == 0)
      sleepq_wake_all((void *)tty_rd->write_buf);

    spinlock_release(tty_rd->slock);
  }

  if(TTY_STATUS_RIRQ(iobase->status)) {
    spinlock_acquire(tty_rd->slock);

    iobase->command = TTY_COMMAND_RIRQ;

    if (TTY_STATUS_ERROR(iobase->status))
      KERNEL_PANIC("Could not issue RIRQ to TTY.");

    while (TTY_STATUS_RAVAIL(iobase->status)) {
      char data = iobase->data;
      int index;

      if (tty_rd->read_count > TTY_BUF_SIZE)
        continue;

      index = (tty_rd->read_head + tty_rd->read_count) % TTY_BUF_SIZE;

      tty_rd->read_buf[index] = data;
      tty_rd->read_count++;
    }

    spinlock_release(tty_rd->slock);
    sleepq_wake_all((void *)tty_rd->read_buf);

  }
}

/**
 * Writes len bytes from buffer buf to tty-device
 * pointed by gcd. Implements write from the gbd interface.
 *
 * @param gcd Pointer to the tty-device.
 * @param buf Buffer to be written from.
 * @param len number of bytes to be written.
 *
 * @return Number of succesfully writeten characters.
 */
static int tty_write(gcd_t *gcd, const void *buf, int len) {
  interrupt_status_t intr_status;
  volatile tty_io_area_t *iobase = (tty_io_area_t *)gcd->device->io_address;
  volatile tty_real_device_t *tty_rd
    = (tty_real_device_t *)gcd->device->real_device;
  int i;

  intr_status = _interrupt_disable();
  spinlock_acquire(tty_rd->slock);

  i = 0;
  while (i < len) {
    while (tty_rd->write_count > 0) {
      /* buffer contains data, so wait until empty. */
      sleepq_add((void *)tty_rd->write_buf);
      spinlock_release(tty_rd->slock);
      thread_switch();
      spinlock_acquire(tty_rd->slock);
    }

    /* Fill internal buffer. */
    while (tty_rd->write_count < TTY_BUF_SIZE  && i < len) {
      int index;
      index = (tty_rd->write_head + tty_rd->write_count) % TTY_BUF_SIZE;
      tty_rd->write_buf[index] = ((char *)buf)[i++];
      tty_rd->write_count++;
    }

    /* If device is not currently busy, write one charater to
       cause interrupt. Head and count are adjusted not to write
       first character twice. Rest of the buffer is written by
       interrupt handler.

       If the device is busy, interrupt will appear by itself and
       the whole buffer will be written by interrupt handler.
    */
    if (!TTY_STATUS_WBUSY(iobase->status)) {
      iobase->data = tty_rd->write_buf[tty_rd->write_head];
      tty_rd->write_head = (tty_rd->write_head + 1) % TTY_BUF_SIZE;
      tty_rd->write_count--;
    }

  }

  spinlock_release(tty_rd->slock);
  _interrupt_set_state(intr_status);

  return i;
}


/**
 * Reads atmost len bytes from tty-device pointed by
 * gcd to buffer buf.
 *
 * @param gcd Pointer to the tty-device.
 * @param buf Character buffer to be read into.
 * @param len Maximum number of bytes to be read.
 *
 * @return Number of succesfully read characters.
 */
static int tty_read(gcd_t *gcd, void *buf, int len) {
  interrupt_status_t intr_status;
  volatile tty_real_device_t *tty_rd
    = (tty_real_device_t *)gcd->device->real_device;
  int i;

  intr_status = _interrupt_disable();
  spinlock_acquire(tty_rd->slock);

  while (tty_rd->read_count == 0) {
    /* buffer is empty, so wait it to be filled */
    sleepq_add((void *)tty_rd->read_buf);
    spinlock_release(tty_rd->slock);
    thread_switch();
    spinlock_acquire(tty_rd->slock);
  }


  /* Data is read to internal buffer by interrupt driver. Number of
     chars read is stored to i. */
  i = 0;
  while (tty_rd->read_count > 0 && i < len) {
    ((char *)buf)[i++] = tty_rd->read_buf[tty_rd->read_head];
    tty_rd->read_head = (tty_rd->read_head + 1) % TTY_BUF_SIZE;
    tty_rd->read_count--;
  }

  spinlock_release(tty_rd->slock);
  _interrupt_set_state(intr_status);

  return i;
}

/** @} */
