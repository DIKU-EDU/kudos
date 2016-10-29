/*
 * Spinlocks
 */

#ifndef KUDOS_KERNEL_SPINLOCK_H
#define KUDOS_KERNEL_SPINLOCK_H

typedef int spinlock_t;

void spinlock_reset(spinlock_t *slock);
void spinlock_acquire(spinlock_t *slock);
void spinlock_release(spinlock_t *slock);

#endif // KUDOS_KERNEL_SPINLOCK_H
