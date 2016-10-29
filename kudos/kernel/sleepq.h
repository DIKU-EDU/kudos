/*
 * Sleep queue
 */

#ifndef KUDOS_KERNEL_SLEEPQ_H
#define KUDOS_KERNEL_SLEEPQ_H

/* Prototypes for sleep queue functions */
void sleepq_init(void);
void sleepq_add(void *resource);
void sleepq_wake(void *resource);
void sleepq_wake_all(void *resource);

#endif // KUDOS_KERNEL_SLEEPQ_H
