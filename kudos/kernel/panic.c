/*
 * Kernel panic.
 */

#include "lib/libc.h"
#include "drivers/metadev.h"
#include "kernel/interrupt.h"

/** @name Kernel panic
 *
 *   This module implements kernel panic. Kernel panic is a situation
 *   where kernel doesn't know what it should do next (unexpected
 *   error, internal error). Panic causes machine to shut down.
 *
 *   @{
 */

/**
 *   Causes kernel panic. In kernel panic the system stops immediately
 *   after description text is printed to screen. This function is
 *   usually called from KERNEL_PANIC macro, which fills the source
 *   code file and line number arguments automatically.
 *
 *   @param file Filename of the module where panic has been called.
 *
 *   @param line Line-number in file where panic has been called.
 * 
 *   @param description Descriptive text that gets printed on screen
 *   before dying.
 *
 */

void _kernel_panic(char *file, int line, char *description)
{
    int cpu;

    _interrupt_disable();
    cpu = _interrupt_getcpu();
    kprintf("Kernel panic (cpu: %d): %s:%d --- %s\n", cpu, file, line, 
            description);
    shutdown(DEFAULT_SHUTDOWN_MAGIC);
}

/** @} */
