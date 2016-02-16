/*
 * Disk driver
 */

#include <arch.h>
#include "kernel/stalloc.h"
#include "kernel/panic.h"
#include "kernel/assert.h"
#include "kernel/semaphore.h"
#include "kernel/spinlock.h"
#include "kernel/interrupt.h"
#include "lib/libc.h"
#include "drivers/device.h"
#include "drivers/gbd.h"
#include "drivers/disk.h"
#include "drivers/disksched.h"


/**@name Disk driver
 *
 * This module contains functions for disk driver.
 *
 * @{
 */


static void disk_interrupt_handle(device_t *device);
static int disk_read_block(gbd_t *gbd, gbd_request_t *request);
static int disk_write_block(gbd_t *gbd, gbd_request_t *request);
static int disk_submit_request(gbd_t *gbd, gbd_request_t *request);
static void disk_next_request(gbd_t *gbd);
static uint32_t disk_block_size(gbd_t *gbd);
static uint32_t disk_total_blocks(gbd_t *gbd);


/**
 * Initialize disk device driver. Reserves memory for data structures
 * and register driver to the interrupt handler.
 *
 * @param desc Pointer to the YAMS IO device descriptor of the disk
 *
 * @return Pointer to the device structure of the disk
 */
device_t *disk_init(io_descriptor_t *desc) {
  device_t *dev;
  gbd_t    *gbd;
  disk_real_device_t *real_dev;
  uint32_t irq_mask;

  dev = (device_t*)stalloc(sizeof(device_t));
  gbd = (gbd_t*)stalloc(sizeof(gbd_t));
  real_dev = (disk_real_device_t*)stalloc(sizeof(disk_real_device_t));
  if (dev == NULL || gbd == NULL || real_dev == NULL)
    KERNEL_PANIC("Could not allocate memory for disk driver.");

  dev->generic_device = gbd;
  dev->real_device = real_dev;
  dev->descriptor = desc;
  dev->io_address = desc->io_area_base;
  dev->type = desc->type;

  gbd->device = dev;
  gbd->read_block = disk_read_block;
  gbd->write_block = disk_write_block;
  gbd->block_size = disk_block_size;
  gbd->total_blocks = disk_total_blocks;

  spinlock_reset(&real_dev->slock);
  real_dev->request_queue = NULL;
  real_dev->request_served = NULL;

  irq_mask = 1 << (desc->irq + 10);
  interrupt_register(irq_mask, disk_interrupt_handle, dev);

  return dev;
}

/**
 * Disk interrupt handler. Interrupt is raised so request is handled
 * by the disk. Sets return value of current request to zero, wakes up
 * function that is waiting this request and puts next request in work
 * by calling disk_next_request().
 *
 * @param device Pointer to the device data structure
 */
static void disk_interrupt_handle(device_t *device) {
  disk_real_device_t *real_dev = device->real_device;
  disk_io_area_t *io = (disk_io_area_t *)device->io_address;

  spinlock_acquire(&real_dev->slock);

  /* Check if this interrupt was for us */
  if (!(DISK_STATUS_RIRQ(io->status) || DISK_STATUS_WIRQ(io->status))) {
    spinlock_release(&real_dev->slock);
    return;
  }

  /* Just reset both flags, since the handling is identical */
  io->command = DISK_COMMAND_WIRQ;
  io->command = DISK_COMMAND_RIRQ;

  /* If this assert fails, disk has caused an interrupt without any
     service request. */
  KERNEL_ASSERT(real_dev->request_served != NULL);

  real_dev->request_served->return_value = 0;

  /* Wake up the function that is waiting this request to be
     handled.  In case of synchronous request that is
     disk_submit_request. In case of asynchronous call it is
     some other function.*/
  semaphore_V(real_dev->request_served->sem);
  real_dev->request_served = NULL;
  disk_next_request(device->generic_device);

  spinlock_release(&real_dev->slock);
}


/**
 * Reads one block pointed by request from disk pointed by
 * gbd. Operation field of request is set to READ and request is
 * submitted to disk scheduler.Implements gbd's read_block() function.
 *
 * @param gbd Pointer to the gbd data structure.
 *
 * @param request Pointer to the request data structure containing
 * information about block to be read.
 *
 * @return Returns 1 if success, 0 otherwise
 */
static int disk_read_block(gbd_t *gbd, gbd_request_t *request) {
  request->operation = GBD_OPERATION_READ;
  return disk_submit_request(gbd, request);
}


/**
 * Writes one block pointed by request to disk pointed by
 * gbd. Operation field of request is set to WRITE and request is
 * submitted to disk scheduler.Implements gbd's write_block() function.
 *
 * @param gbd Pointer to the gbd data structure.
 *
 * @param request Pointer to the request data structure containing
 * information about block to be written.
 *
 * @return Returns 1 if success, 0 otherwise
 */
static int disk_write_block(gbd_t *gbd, gbd_request_t *request)
{
  request->operation = GBD_OPERATION_WRITE;
  return disk_submit_request(gbd, request);
}


/**
 * Submits a request to the request queue. Request is inserted in the
 * queue by disk scheduler.
 *
 * If request is synchronous (request->sem == NULL) call will block
 * and wait until the request is handled. Appropriate return value is
 * returned.
 *
 * If request is asynchronous (request->sem != NULL) call will return
 * immediately. 1 will be returned as retrun value.
 *
 * @param gbd Pointer to the gbd-device that will hadle request
 *
 * @param request Pointer to the request to be handled.
 *
 * @return 1 if success, 0 otherwise.
 */
static int disk_submit_request(gbd_t *gbd, gbd_request_t *request) {
  int sem_null;
  interrupt_status_t intr_status;
  disk_real_device_t *real_dev = gbd->device->real_device;

  request->internal = NULL;
  request->next     = NULL;
  request->return_value = -1;

  sem_null = (request->sem == NULL);
  if(sem_null) {
    /* Semaphore is null so this is synchronous request.
       Create a new semaphore with value 0. This will cause
       this function to block until the interrupt handler has
       handled the request.
    */
    request->sem = semaphore_create(0);
    if(request->sem == NULL)
      return 0;   /* failure */
  }

  intr_status = _interrupt_disable();
  spinlock_acquire(&real_dev->slock);

  disksched_schedule(&real_dev->request_queue, request);

  if(real_dev->request_served == NULL) {
    /* Driver is idle so new request under work */
    disk_next_request(gbd);
  }

  spinlock_release(&real_dev->slock);
  _interrupt_set_state(intr_status);

  if(sem_null) {
    /* Synchronous call. Wait here until the interrupt handler has
       handled the request. After this semaphore created earlier
       in this function is no longer needed. */
    semaphore_P(request->sem);
    semaphore_destroy(request->sem);
    request->sem = NULL;

    /* Request is handled. Check the retrun value. */
    if(request->return_value == 0)
      return 1;
    else
      return 0;

  } else {
    /* Asynchronous call. Assume success, because request is not yet
       handled. */
    return 1;
  }
}


/**
 * Gets one request from request queue and puts the disk in
 * work. Assumes that interrupts are disabled and device spinlock is
 * held. Also assumes that the device is idle.
 *
 * @param gbd pointer to the general block device.
 */
static void disk_next_request(gbd_t *gbd) {
  disk_real_device_t *real_dev = gbd->device->real_device;
  disk_io_area_t *io = (disk_io_area_t *)gbd->device->io_address;
  volatile gbd_request_t *req;

  KERNEL_ASSERT(!(DISK_STATUS_RBUSY(io->status) ||
                  DISK_STATUS_WBUSY(io->status)));
  KERNEL_ASSERT(real_dev->request_served == NULL);

  req = real_dev->request_queue;
  if(req == NULL) {
    /* There were no requests. */
    return;
  }
  real_dev->request_queue = req->next;
  req->next = NULL;

  real_dev->request_served = req;


  io->tsector = req->block;
  io->dmaaddr = (uint32_t)req->buf;
  if(req->operation == GBD_OPERATION_READ) {
    io->command = DISK_COMMAND_READ;
  } else if(req->operation == GBD_OPERATION_WRITE) {
    io->command = DISK_COMMAND_WRITE;
  } else {
    KERNEL_PANIC("disk_next_request: Unknown gbd operation.");
  }

  if(DISK_STATUS_ERRORS(io->status)) {
    kprintf("disk error: 0x%8.8x\n", DISK_STATUS_ERRORS(io->status));
    KERNEL_PANIC("disk error occured");
  }
}


/**
 * Returns blocksize of disk pointed by gbd. Implements gbd's block_size()
 * function.
 *
 * @param gbd Pointer to gbd data structure.
 *
 * @return Block size in bytes of the disk.
 */
static uint32_t disk_block_size(gbd_t *gbd) {
  interrupt_status_t intr_status;
  disk_real_device_t *real_dev = gbd->device->real_device;
  disk_io_area_t *io = (disk_io_area_t *)gbd->device->io_address;
  uint32_t ret;


  intr_status = _interrupt_disable();
  spinlock_acquire(&real_dev->slock);

  io->command = DISK_COMMAND_BLOCKSIZE;
  ret = io->data;

  spinlock_release(&real_dev->slock);
  _interrupt_set_state(intr_status);

  return ret;
}


/**
 * Returns number of blocks of disk pointed by gbd. Implements gbd's
 * total_blocks() function.
 *
 * @param gbd Pointer to the gbd data structure.
 *
 * @return Number of blocks of the disk.
 */
static uint32_t disk_total_blocks(gbd_t *gbd) {
  interrupt_status_t intr_status;
  disk_real_device_t *real_dev = gbd->device->real_device;
  disk_io_area_t *io = (disk_io_area_t *)gbd->device->io_address;
  uint32_t ret;


  intr_status = _interrupt_disable();
  spinlock_acquire(&real_dev->slock);

  io->command = DISK_COMMAND_BLOCKS;
  ret = io->data;

  spinlock_release(&real_dev->slock);
  _interrupt_set_state(intr_status);

  return ret;
}

/** @} */
