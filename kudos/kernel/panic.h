/*
 * Kernel panic.
 */

#ifndef KUDOS_KERNEL_PANIC_H
#define KUDOS_KERNEL_PANIC_H

/* Causes kernel panic with descriptive text "desc" */
#define KERNEL_PANIC(desc) _kernel_panic(__FILE__, __LINE__, desc)

/* Causes kernel panic. Prints filename, line and description before
   dropping the system into YAMS hardware console. */
void _kernel_panic(char *file, int line, char *description);

#endif // KUDOS_KERNEL_PANIC_H
