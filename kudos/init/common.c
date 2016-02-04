/*
 * Common entry routines
 */

#include "init/common.h"
#include <arch.h>
#include "drivers/bootargs.h"
#include "drivers/device.h"
#include "drivers/gcd.h"
#include "drivers/metadev.h"
#include "drivers/polltty.h"
#include "fs/vfs.h"
#include "kernel/assert.h"
#include "kernel/config.h"
#include "kernel/halt.h"
#include "kernel/idle.h"
#include "kernel/interrupt.h"
#include "kernel/stalloc.h"
#include "kernel/panic.h"
#include "kernel/scheduler.h"
#include "kernel/synch.h"
#include "kernel/thread.h"
#include "lib/debug.h"
#include "lib/libc.h"
#include "proc/process.h"
#include "vm/memory.h"

/**
 * Fallback function for system startup. This function is executed
 * if the initial startup program (shell or other userland process given
 * with initprog boot argument) is not started or specified.
 *
 * This is a good place for some kernel test code!
 *
 */

void init_startup_fallback(void) {

  DEBUG("debuginit", "In init_startup_fallback\n");

  /* Run console test if "testconsole" was given as boot argument. */
  if (bootargs_get("testconsole") != NULL) {
    device_t *dev;
    gcd_t *gcd;
    char buffer[64];
    char buffer2[64];
    int len;

    DEBUG("debuginit", "In console test\n");

    /* Find system console (first tty) */
    dev = device_get(TYPECODE_TTY, 0);
    KERNEL_ASSERT(dev != NULL);


    gcd = (gcd_t *)dev->generic_device;
    KERNEL_ASSERT(gcd != NULL);

    len = snprintf(buffer, 63, "Hello user! Press any key.\n");
    gcd->write(gcd, buffer, len);

    len = gcd->read(gcd, buffer2, 63);
    KERNEL_ASSERT(len >= 0);
    buffer2[len] = '\0';

    len = snprintf(buffer, 63, "You said: '%s'\n", buffer2);
    gcd->write(gcd, buffer, len);

    DEBUG("debuginit", "Console test done, %d bytes written\n", len);
  }

  /* Nothing else to do, so we shut the system down. */
  kprintf("Startup fallback code ends.\n");
  halt_kernel();
}

/**
 * Initialize the system. This function is called from the first
 * system thread fired up by the boot code below.
 *
 * @param arg Dummy argument, unused.
 */

void init_startup_thread(uint32_t arg)
{
  /* Threads have arguments for functions they run, we don't
     need any. Silence the compiler warning by using the argument. */
  arg = arg;

  kprintf("Mounting filesystems\n");
  vfs_mount_all();

  if(bootargs_get("initprog") == NULL) {
    kprintf("No initial program (initprog), dropping to fallback\n");
    init_startup_fallback();
  }

  kprintf("Starting initial program '%s'\n", bootargs_get("initprog"));

  process_start(bootargs_get("initprog"), NULL);

  /* The current process_start() should never return. */
  KERNEL_PANIC("Run out of initprog.\n");
}
