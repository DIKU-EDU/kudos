/*
 * Generic network device
 */

#ifndef KUDOS_DRIVERS_GND_H
#define KUDOS_DRIVERS_GND_H

#include "drivers/device.h"

typedef uint32_t network_address_t;

/* Generic network device descriptor. */
typedef struct gnd_struct {
    /* Pointer to the device */
    device_t *device;

    /* Pointer to a function which sends one network frame to addr.
     * The frame must be in the format defined by the media. (For YAMS
     * network that means that the first 8 bytes are filled in the
     * network layer and the rest is data). The frame must have the
     * size returned by the function frame_size below. The call of
     * this function will block until the frame is sent. The return
     * value 0 means success. Other values are failures.
     *
     * Note: The pointer to the frame must be a PHYSICAL address, not
     * a segmented one.
     */
    int (*send)(struct gnd_struct *gnd, void *frame, network_address_t addr);

    /* Pointer to a function which receives one network frame. The
     * frame will be in the format defined by the media. (For YAMS
     * network that means that the first 8 bytes are filled in the
     * network layer and the rest is data). The frame must have the
     * size returned by the function frame_size below. The call of
     * this function will block until a frame is received. The return
     * value 0 means success. Other values are failures.
     *
     * Note: The pointer to the frame must be a PHYSICAL address, not
     * a segmented one.
     */
    int (*recv)(struct gnd_struct *gnd, void *frame);

    /* Pointer to a function which returns the size of the network
     * frame for the media in octets.
     */
    uint32_t (*frame_size)(struct gnd_struct *gnd);

    /* Pointer to a function which returns the hardware address (MAC)
     * of the interface.
     */
    network_address_t (*hwaddr)(struct gnd_struct *gnd);

} gnd_t;

#endif // KUDOS_DRIVERS_GND_H
