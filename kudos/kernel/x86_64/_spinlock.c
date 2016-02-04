/*
 * Spinlock implementation
 */

#include "kernel/spinlock.h"

/* Initialises the spinlock */
void spinlock_reset(spinlock_t *slock)
{
  *slock = 0;
}

/* Release lock by setting it to 0 */
void spinlock_release(spinlock_t *slock)
{
  *slock = 0;
}
