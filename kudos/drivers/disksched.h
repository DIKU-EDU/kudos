/*
 * Disk scheduler
 */

#ifndef KUDOS_DRIVERS_DISKSCHED_H
#define KUDOS_DRIVERS_DISKSCHED_H


#include "drivers/gbd.h"

int disksched_schedule(volatile gbd_request_t **queue,
                       gbd_request_t *request);

#endif // KUDOS_DRIVERS_DISKSCHED_H
