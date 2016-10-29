/*
 * Devices.
 */

#ifndef KUDOS_DRIVERS_DEVICE_H
#define KUDOS_DRIVERS_DEVICE_H

#include <arch.h>
#include "lib/types.h"

/* The device driver instance structure */
typedef struct {
  /* Pointer to device driver's internal data */
  void *real_device;

  /* Pointer to generic device handle. NULL if not implemented by
     the driver */
  void *generic_device;

  /* Pointer to the device descriptor record */
  io_descriptor_t *descriptor;

  /* Start of the memory-mapped io area of the device */
  uint32_t io_address;

  /* The typecode of this device */
  uint32_t type;
} device_t;

/* Shared typecodes */
#define TYPECODE_TTY            0x201
#define TYPECODE_DISK           0x301
#define TYPECODE_NIC            0x401

void device_init(void);
device_t *device_get(uint32_t typecode, uint32_t n);
int device_register(device_t *device);

#endif // KUDOS_DRIVERS_DEVICE_H
