/*
 * Generic character device
 */

#ifndef DRIVERS_GCD_H
#define DRIVERS_GCD_H

#include "drivers/device.h"

/* Generic character device descriptor. */
typedef struct gcd_struct {
    /* Pointer to the device */
    device_t *device;

    /* Pointer to a function which writes len bytes from buf to the
       device. This function returns the number of bytes successfully
       written. Note the call can block. */
    int (*write)(struct gcd_struct *gcd, const void *buf, int len);

    /* Pointer to a function which reads at most len bytes from the
       device to buf. The function returns the number of bytes read.
       Note the call can block. */
    int  (*read)(struct gcd_struct *gcd, void *buf, int len);
} gcd_t;

#endif /* DRIVERS_GCD_H */
