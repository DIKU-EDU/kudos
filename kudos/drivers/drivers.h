/*
 * Drivers
 */

#ifndef KUDOS_DRIVERS_DRIVERS_H
#define KUDOS_DRIVERS_DRIVERS_H

#include "lib/types.h"
#include "drivers/device.h"

/* A structure containing available drivers */
typedef struct {
    /* The type of the device */
    uint32_t typecode;

    /* The name of the driver */
    const char *name;

    /* The initialization function for the driver */
    device_t *(*initfunc)(io_descriptor_t *descriptor);
} drivers_available_t;

extern drivers_available_t drivers_available[];

#endif // KUDOS_DRIVERS_DRIVERS_H
