/*
 * Scheduler.
 */

#ifndef KUDOS_KERNEL_SCHEDULER_H
#define KUDOS_KERNEL_SCHEDULER_H

#include "kernel/thread.h"

/* function definitions */
void scheduler_init(void);
void scheduler_add_ready(TID_t t);
void scheduler_schedule(void);

#endif // KUDOS_KERNEL_SCHEDULER_H
