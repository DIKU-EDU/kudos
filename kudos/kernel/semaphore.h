/*
 * Semaphores
 */

#ifndef KUDOS_KERNEL_SEMAPHORE_H
#define KUDOS_KERNEL_SEMAPHORE_H

#include "kernel/spinlock.h"
#include "kernel/thread.h"

typedef struct {
    spinlock_t slock;
    int value;
    TID_t creator;
} semaphore_t;

void semaphore_init(void);
semaphore_t *semaphore_create(int value);
void semaphore_destroy(semaphore_t *sem);
void semaphore_P(semaphore_t *sem);
void semaphore_V(semaphore_t *sem);

#endif // KUDOS_KERNEL_SEMAPHORE_H
