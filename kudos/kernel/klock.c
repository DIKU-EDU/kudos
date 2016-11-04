/// Kernel lock. A convenient wrapper.

#include "kernel/klock.h"

void
klock_init(klock_t *klock) {
  spinlock_reset(klock);
}

klock_status_t
klock_lock(klock_t *klock) {
  interrupt_status_t st = _interrupt_disable();
  spinlock_acquire(klock);
  return st;
}

void
klock_open(klock_status_t st, klock_t *klock) {
  spinlock_release(klock);
  _interrupt_set_state(st);
}
