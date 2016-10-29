/*
 * Kernel boot arguments.
 */

#ifndef KUDOS_DRIVERS_BOOTARGS_H
#define KUDOS_DRIVERS_BOOTARGS_H

void bootargs_init(void *bootargs);
char *bootargs_get(char *key);

#endif // KUDOS_DRIVERS_BOOTARGS_H
