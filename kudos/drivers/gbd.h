/*
 * Generic block device
 */

#ifndef KUDOS_DRIVERS_GBD_H
#define KUDOS_DRIVERS_GBD_H

#include "lib/libc.h"
#include "drivers/device.h"
#include "kernel/semaphore.h"

/* Operation codes for Generic Block Device requests. */

typedef enum {
    GBD_OPERATION_READ,
    GBD_OPERATION_WRITE
} gbd_operation_t;

/**
 * Block Device Request Descriptor. When using generic block device
 * read or write functions a pointer to this structure is given as
 * argument. Fill fields block, buf and sem before calling GBD
 * functions, the rest are used only internally in the driver.
 *
 * Note that when you call asynchronic versions of read_block or
 * write_block (sem is not NULL), this structure must stay in memory
 * until the operation is complete (the semaphore is raised). Be
 * careful when allocating this structure from stack.
 *
 */

typedef struct gbd_request_struct {
    /* Block number to operate on */
    uint32_t        block;

    /* Pointer to buffer whose size is equal to block size of the device.

       Note that this pointer must be a PHYSICAL address, not
       segmented address.
    */
    uint32_t       buf;

    /* Semaphore which is signaled (increased by one) when the operation
       is complete. If this is set to NULL in call of read or write,
       the call will block until the request is complete.
    */
    semaphore_t    *sem;

    /* Operation code for the request. Filled by the driver. */
    gbd_operation_t operation;

    /* Driver internal data. */
    void           *internal;

    /* Changing pointer for request queues. Used internally by drivers. */ 
    struct gbd_request_struct *next;

    /* Return value for asynchronous call of read or write. After
       the sem is signaled, return value can be read from this field. 
       0 is success, other values indicate failure. */
    int             return_value;
} gbd_request_t;

/* Generic block device descriptor. */
typedef struct gbd_struct {
    /* Pointer to the real device driver under this interface. */
    device_t       *device;

    /* A pointer to a function which reads one block from the device.
       
       Before calling, fill fields block, buf and sem in request.
       If sem is set to NULL, this call will block until the
       request is complete (a block is read). If sem is not NULL,
       this function will return immediately and sem is signaled
       when the request is complete.
    */
    int (*read_block)(struct gbd_struct *gbd, gbd_request_t *request);

    /* A pointer to a function which writes one block to the device.
       
       Before calling, fill fields block, buf and sem in request.
       If sem is set to NULL, this call will block until the
       request is complete (a block is written). If sem is not NULL,
       this function will return immediately and sem is signaled
       when the request is complete.
    */
    int (*write_block)(struct gbd_struct *gbd, gbd_request_t *request);

    /* A pointer to a function which returns the block size of the device
       in bytes. */
    uint32_t (*block_size)(struct gbd_struct *gbd);

    /* A pointer to a function which returns the total number of 
       blocks in this device. */
    uint32_t (*total_blocks)(struct gbd_struct *gbd);
} gbd_t;


#endif // KUDOS_DRIVERS_GBD_H
