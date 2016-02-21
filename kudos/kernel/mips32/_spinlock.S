/*
 * Spinlocks
 */

#include "lib/registers.h"

  .text
  .align  2
/*
 * The initialization and releasing functions for spinlocks do the same
 * thing. Therefore the symbols spinlock_reset and spinlock_release map
 * to the same address. They both set the value of the spinlock to zero
 * (ie. the spinlock is not held by any cpu).
 */

# void spinlock_reset(spinlock_t *slock)
# void spinlock_release(spinlock_t *slock)
  .globl  spinlock_reset
  .globl  spinlock_release
  .ent  spinlock_reset

spinlock_reset:
spinlock_release:
  sw  zero, (a0)
  jr  ra
  .end  spinlock_reset

/* Acquire a spinlock. Sets the value of the spinlock to one indicating
 * that the spinlock is reserved. We need MIPS32 special instructions LL
 * SC to implement this on an SMP.
 */

# void spinlock_acquire(spinlock_t *slock)
  .globl  spinlock_acquire
  .ent  spinlock_acquire

spinlock_acquire:
  ll  t0, (a0)
  bnez  t0, spinlock_acquire
  li  t0, 1
  sc  t0, (a0)
  beqz  t0, spinlock_acquire
  jr  ra
  .end  spinlock_acquire



