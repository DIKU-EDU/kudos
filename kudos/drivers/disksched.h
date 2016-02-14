/*
 * Disk scheduler
 */

#ifndef DRIVERS_DISKSCHED_H
#define DRIVERS_DISKSCHED_H


#include "drivers/gbd.h"

int disksched_schedule(volatile gbd_request_t **queue,
                       gbd_request_t *request);

#endif /* DRIVERS_DISKSCHED_H */
