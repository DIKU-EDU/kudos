/*
 * Scheduler.
 */

#ifndef KUdos_KERNEL_SCHEDULER_H
#define KUdos_KERNEL_SCHEDULER_H

#include "kernel/thread.h"

/* function definitions */
void scheduler_init(void);
void scheduler_add_ready(TID_t t);
void scheduler_schedule(void);

#endif /* KUdos_KERNEL_SCHEDULER_H */
