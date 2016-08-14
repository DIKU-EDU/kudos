/*
 * Devices.
 */

#include <pit.h>
#include <pci.h>
#include <asm.h>
#include "lib/libc.h"
#include "drivers/device.h"
#include "kernel/config.h"
#include "drivers/drivers.h"
#include "drivers/disk.h"
#include "kernel/interrupt.h"
#include <keyboard.h>

/**@name Device Drivers
 *
 * This module contains functions for initializing device drivers and
 * maintaining a list of found devices.
 *
 * @{
 */

/* Extern */
extern device_t *tty_dev;

/** Table of initialized device drivers. */
static device_t *device_table[CONFIG_MAX_DEVICES];

/** Number of initialized device drivers. */
static int number_of_devices = 0;

/**
 * Initializes all device drivers. Tries to find a driver for each
 * device in the system and initializes the found drivers.
 *
 * Initialized drivers are stored to device_table and can be
 * accessed by calling device_get.
 *
 */

void device_init(void)
{
    /* Map tty */
    device_table[number_of_devices] = tty_dev;
    number_of_devices++;

    /* Install Timer */
    pit_init();

    /* Enable interrupts */
    _interrupt_enable();

    /* Install PS-2 Keyboard */
    keyboard_init();
}

/**
 * Gets device driver of a given device.
 *
 * @param typecode The typecode of the device
 *
 * @param n The n:th device of given type is returned. Indexing begins
 * from 0.
 *
 * @return The device driver. NULL if not found or n too large.
 */
device_t *device_get(uint32_t typecode, uint32_t n)
{
    int i;

    for(i = 0; i < number_of_devices; i++) {
        if (device_table[i]->type == typecode) {
            if (n == 0)
                return device_table[i];
            else
                n--;
        }
    }
    return NULL;
}

/*
 * Register new device
 *
 * @param device The device to register
 *
 * @return 0 for success, -1 for failure
 */
int device_register(device_t *device){
    if(number_of_devices >= CONFIG_MAX_DEVICES)
        return -1;

    device_table[number_of_devices] = device;
    number_of_devices++;
    return 0;
}

/** @} */
