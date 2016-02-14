/*
 * Disk scheduler
 */


#include "drivers/gbd.h"


/**@name Disk scheduler
 *
 * @{
 */

/**
 * Schedules a disk operation. Currently puts the new request to the
 * end of request queue.
 */
void disksched_schedule(volatile gbd_request_t **queue,
                        gbd_request_t *request)
{
  volatile gbd_request_t *q;
  q = *queue;

  if(q != NULL) {
    while((q)->next != NULL) {
      q = (q)->next;
    }
    q->next = request;
  } else {
    *queue = request;
  }
}

/** @} */
