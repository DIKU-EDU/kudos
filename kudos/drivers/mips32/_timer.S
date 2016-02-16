/*
 * CP0 timer handling
 */

#include "lib/registers.h"

  .text
  .align  2

# void _timer_set_ticks(uint32_t ticks);
# 
# Sets the hardware timer to fire hardware interrupt 5 after
# given number of ticks.

  .globl  _timer_set_ticks
  .ent  _timer_set_ticks

_timer_set_ticks:
  mfc0  t0, Count, 0
  addu  a0, t0, a0
  mtc0  a0, Compar, 0
  j     ra
 .end   _timer_set_ticks
