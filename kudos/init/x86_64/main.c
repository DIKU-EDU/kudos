/*
 * Main startup routines for KUDOS
 */

#include <multiboot.h>
#include "init/common.h"
#include "kernel/interrupt.h"
#include "vm/memory.h"
#include "lib/types.h"
#include "lib/libc.h"
#include "drivers/polltty.h"
#include "kernel/stalloc.h"
#include "kernel/thread.h"
#include "kernel/sleepq.h"
#include "kernel/semaphore.h"
#include "kernel/scheduler.h"
#include "drivers/device.h"
#include "drivers/bootargs.h"
#include "fs/vfs.h"
#include <keyboard.h>

/**
 * Initialize the system. This function is called by CPU0 just
 * after the kernel code is entered first time after boot.
 *
 * The system is still in bootup-mode and can't run threads or multiple
 * CPUs.
 *
 */

int init(uint64_t magic, uint8_t *multiboot)
{
  /* Setup Static Allocation System */
  multiboot_info_t *mb_info = (multiboot_info_t*)multiboot;
  TID_t startup_thread;
  stalloc_init();

  /* Setup video printing */
  polltty_init();

  kwrite("Kudos is an educational operating system by the University of Copenhagen\n");
  kwrite("========================================================================\n");
  kwrite("Based on the Buenos operating system skeleton\n");
  kwrite("\n");

  kprintf("Copyright (C) 2003-2016  Juha Aatrokoski, Timo Lilja,\n");
  kprintf("  Leena Salmela, Teemu Takanen, Aleksi Virtanen, Philip Meulengracht,\n");
  kprintf("  Troels Henriksen, Annie Jane Pinder, Niels Gustav Westphal Serup,\n");
  kprintf("  Nicklas Warming Jacobsen, Oleksandr Shturmov.\n");
  kprintf("See the file COPYING for licensing details.\n");
  kprintf("\n");

  /* Setup GDT/IDT/Exceptions */
  kprintf("Initializing interrupt handling\n");
  interrupt_init(1);

  /* Read boot args */
  kprintf("Reading boot arguments\n");
  bootargs_init((void*)(uint64_t)mb_info->cmdline);

  /* Setup Memory */
  kprintf("Initializing memory system\n");
  physmem_init(multiboot);
  vm_init();

  /* Seed the random number generator. */
  if (bootargs_get("randomseed") == NULL) {
    _set_rand_seed(0);
  } else {
    int seed = atoi((char*)(uint64_t)bootargs_get("randomseed"));
    kprintf("Seeding pseudorandom number generator with %i\n", seed);
    _set_rand_seed(seed);
  }

  /* Setup Threading */
  kprintf("Initializing threading table\n");
  thread_table_init();

  kprintf("Initializing sleep queue\n");
  sleepq_init();

  kprintf("Initializing semaphores\n");
  semaphore_init();

  /* Start scheduler */
  kprintf("Initializing scheduler\n");
  scheduler_init();

  /* Setup Drivers */
  kprintf("Initializing device drivers\n");
  device_init();

  kprintf("Initializing virtual filesystem\n");
  vfs_init();

  kprintf("Creating initialization thread\n");
  startup_thread = thread_create(init_startup_thread, 0);
  thread_run(startup_thread);

  kprintf("Starting threading system and SMP\n");

  /* Enter context switch, scheduler will be run automatically,
     since thread_switch() behaviour is identical to timer tick
     (thread timeslice is over). */
  thread_switch();

  return 0xDEADBEEF;
}
