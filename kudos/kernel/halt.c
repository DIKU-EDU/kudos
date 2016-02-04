/*
 * Halt the system
 */
#include "kernel/halt.h"
#include "drivers/metadev.h"
#include "lib/libc.h"
#include "fs/vfs.h"

/**
 * Halt the kernel.
 */
void halt_kernel(void)
{
    kprintf("Kernel: System shutdown started...\n");

    /* Unmount all filesystems */
    vfs_deinit();

    kprintf("Kernel: System shutdown complete, powering off\n");
    shutdown(POWEROFF_SHUTDOWN_MAGIC);
}
