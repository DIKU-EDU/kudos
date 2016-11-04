/// Kernel lock. A convenient wrapper.

#ifndef KUDOS_KERNEL_KLOCK_H
#define KUDOS_KERNEL_KLOCK_H

#include "kernel/interrupt.h"
#include "kernel/spinlock.h"

typedef interrupt_status_t klock_status_t;

typedef spinlock_t klock_t;

void klock_init(klock_t *klock);

klock_status_t klock_lock(klock_t *klock);
void klock_open(klock_status_t, klock_t *klock);

#endif // KUDOS_KERNEL_KLOCK_H
