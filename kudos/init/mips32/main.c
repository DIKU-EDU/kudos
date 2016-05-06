/*
 * Main startup routines for KUDOS
 */

#include <arch.h>
#include "init/common.h"
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

/* Whether other processors than 0 may continue in SMP mode.
   CPU0 runs the actual init() below, other CPUs loop and wait
   this variable to be set before they will enter context switch
   and scheduler to get a thread to run. */
int kernel_bootstrap_finished = 0;


/**
 * Initialize the system. This function is called by CPU0 just
 * after the kernel code is entered first time after boot.
 *
 * The system is still in bootup-mode and can't run threads or multiple
 * CPUs.
 *
 */
void init(void)
{
  TID_t startup_thread;
  int numcpus;

  // Initialise static allocation.
  stalloc_init();

  // Initialize polling TTY driver for kprintf() usage.
  polltty_init();

  kwrite("KUDOS - a skeleton OS for exploring OS concepts\n");
  kwrite("===============================================\n");
  kwrite("\n");
  kwrite("KUDOS is heavily based on BUENOS.\n");
  kwrite("\n");
  kwrite("Copyright (C) 2015-2016 Troels Henriksen, Annie Jane Pinder,\n");
  kwrite("      Niels Gustav Westphal Serup, Oleksandr Shturmov,\n");
  kwrite("      Nicklas Warming Jacobsen.\n");
  kwrite("\n");
  kwrite("Copyright (C) 2014 Philip Meulengracht.\n");
  kwrite("\n");
  kwrite("Copyright (C) 2003-2012 Juha Aatrokoski, Timo Lilja,\n");
  kwrite("      Leena Salmela, Teemu Takanen, Aleksi Virtanen.\n");
  kwrite("\n");
  kwrite("See the file COPYING for licensing details.\n");
  kwrite("\n");

  kwrite("Reading boot arguments\n");
  bootargs_init((void*)BOOT_ARGUMENT_AREA);

  // Seed the random number generator.
  if (bootargs_get("randomseed") == NULL) {
    _set_rand_seed(0);
  } else {
    int seed = atoi(bootargs_get("randomseed"));
    kprintf("Seeding pseudorandom number generator with %i\n", seed);
    _set_rand_seed(seed);
  }

  numcpus = cpustatus_count();
  kprintf("Detected %i CPUs\n", numcpus);
  KERNEL_ASSERT(numcpus <= CONFIG_MAX_CPUS);

  kwrite("Initializing interrupt handling\n");
  interrupt_init(numcpus);

  kwrite("Initializing threading system\n");
  thread_table_init();

  kwrite("Initializing sleep queue\n");
  sleepq_init();

  kwrite("Initializing semaphores\n");
  semaphore_init();

  kwrite("Initializing device drivers\n");
  device_init();

  kprintf("Initializing virtual filesystem\n");
  vfs_init();

  kwrite("Initializing scheduler\n");
  scheduler_init();

  kwrite("Initializing virtual memory\n");
  vm_init();

  kprintf("Creating initialization thread\n");
  startup_thread = thread_create(&init_startup_thread, 0);
  thread_run(startup_thread);

  kprintf("Starting threading system and SMP\n");

  // Let other CPUs run.
  kernel_bootstrap_finished = 1;

  _interrupt_clear_bootstrap();
  _interrupt_enable();

  // Enter context switch, scheduler will be run automatically,
  // since thread_switch() behaviour is identical to timer tick
  // (thread timeslice is over).
  thread_switch();

  // We should never get here
  KERNEL_PANIC("Threading system startup failed.");
}
