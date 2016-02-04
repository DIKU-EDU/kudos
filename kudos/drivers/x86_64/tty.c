/*
 * TTY driver
 */

#include <asm.h>
#include <tty.h>
#include <keyboard.h>
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

/**@name TTY driver
 *
 * This module contains functions for interrupt driven TTY driver.
 *
 * @{
 */
int tty_write(gcd_t *gcd, const void *buf, int len);
int tty_read(gcd_t *gcd, void *buf, int len);

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
device_t *tty_init(io_descriptor_t *desc) 
{
  device_t *dev;
  gcd_t *gcd;
  tty_t *tty;

  dev = (device_t*) stalloc(sizeof(device_t));
  if (dev == NULL)
    KERNEL_PANIC("Could not reserve memory for tty driver.");

  gcd = (gcd_t*) stalloc(sizeof(gcd_t));
  if (gcd == NULL)
    KERNEL_PANIC("Could not reserve memory for tty driver.");

  /* Setup device descriptor */

  dev->generic_device = gcd;
  dev->io_address     = VIDEO_MEMORY_BASE;
  dev->type           = TYPECODE_TTY;

  gcd->device = dev;
  gcd->write  = tty_write;
  gcd->read   = tty_read;

  tty = (tty_t*) stalloc(sizeof(tty_t));
  if(tty == NULL)
    KERNEL_PANIC("Could not reserve memory for tty driver.");

  /* Setup TTY */
  tty->cursor_x = 0;
  tty->cursor_y = 0;
  tty->textcolor = COLOR_WHITE;

  dev->real_device = tty;

  tty->slock = (spinlock_t*) stalloc(sizeof(spinlock_t));
  spinlock_reset(tty->slock);

  return dev;
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
int tty_write(gcd_t *gcd, const void *buf, int len)
{
  interrupt_status_t intr_status;
  uint64_t vidbase = (uint64_t) VIDEO_MEMORY_BASE;
  uint8_t *str = (uint8_t*) buf;
  tty_t *tty = (tty_t*) gcd->device->real_device;
  int i = 0, j = len;

  intr_status = _interrupt_disable();
  spinlock_acquire(tty->slock);

  /* Print */
  while(j) //*String
    {
      /* Get char? */
      uint8_t c = (uint8_t) *str;

      if(c == 0)
        break;

      /* Foreground color of the video memory is the low 4 bits */
      /* bits 4..7 are background color */
      uint8_t fcolor = COLOR_WHITE;
      uint16_t t_attrib = (uint16_t)(fcolor << 8);

      /* Calculate where to put next character */
      uint64_t offset = (tty->cursor_y * VIDEO_MEMORY_COLUMNS + 
                         tty->cursor_x) * 2;
      volatile uint16_t *vptr = (volatile uint16_t*)(vidbase + offset);

      /* Handle special chars like newline */
      switch(c)
        {
          /* Backspace */
        case 0x08:
          {
            /* Check if we aren't already at 0 cursor */
            if(tty->cursor_x != 0)
              {
                /* Back back one char and delete */
                tty->cursor_x--;
                vptr--;
                *vptr = (uint16_t)(' ' | t_attrib);
              }
          } break;
          /* Carriage Return */
        case '\r':
          {
            /* Reset Cursor X Position */
            tty->cursor_x = 0;
          } break;
          /* Newline */
        case '\n':
          {
            /* Go one up in cursor Y and reset X */
            tty->cursor_y++;
            tty->cursor_x = 0;
          } break;

        default:
          {
            /* Print char */
            *vptr = (uint16_t)(t_attrib | c);

            /* Increase pointer */
            tty->cursor_x++;

          } break;
        }
      /* Post print checks */

      /* Check if we need to advance to next line */
      if(tty->cursor_x >= VIDEO_MEMORY_COLUMNS)
        {
          /* Newline */
          tty->cursor_y++;
          tty->cursor_x = 0;
        }

      /* Do we need to scroll? */
      if(tty->cursor_y >= VIDEO_MEMORY_ROWS)
        {
          /* Move all lines one up */
          uint16_t *sptr = (uint16_t*) vidbase;
          for(i = 0; 
              i < (VIDEO_MEMORY_ROWS-1) * VIDEO_MEMORY_COLUMNS; 
              i++)
            {
              sptr[i] = sptr[i + VIDEO_MEMORY_COLUMNS];
            }

          /* Clear last line */
          for (i = (VIDEO_MEMORY_ROWS-1) * VIDEO_MEMORY_COLUMNS; 
               i < VIDEO_MEMORY_ROWS * VIDEO_MEMORY_COLUMNS;
               i++)
            {
              sptr[i] = (uint16_t)(' ' | t_attrib);
            }

          tty->cursor_y--;
          tty->cursor_x = 0;
        }

      /* Advance in string */
      /* Now, we dont advance in string, since this 
       * function only gets called with len = 1 */
      str++;
      j--;
    }

  /* Last, update hardware cursor */
  uint16_t cursor_loc = tty->cursor_y * VIDEO_MEMORY_COLUMNS + tty->cursor_x;
  _outb(0x3D4, 14);
  _outb(0x3D5, (uint8_t)(cursor_loc >> 8)); /* Send High Byte */
  _outb(0x3D4, 15);
  _outb(0x3D5, (uint8_t)cursor_loc);        /* Send Low Byte */

  spinlock_release(tty->slock);
  _interrupt_set_state(intr_status);

  return len;
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
int tty_read(gcd_t *gcd, void *buf, int len)
{
  int i = 0, j = len;
  char *str = (char*) buf;

  /* Read until escape-sequence */
  while(j)
    {
      /* Get key */
      char key = keyboard_getkey();
        
      /* Cancel on enter /newline */
      if(key == '\n')
        break;

      /* Set buf */
      *str = key;

      /* Increase */
      i++;
      str++;
      j--;
    }
    
  return i;
}

/** @} */
