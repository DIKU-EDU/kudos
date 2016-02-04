/*
 * Kernel boot arguments.
 */

#ifndef DRIVERS_BOOTARGS_H
#define DRIVERS_BOOTARGS_H

void bootargs_init(void *bootargs);
char *bootargs_get(char *key);

#endif /* DRIVERS_BOOTARGS_H */
