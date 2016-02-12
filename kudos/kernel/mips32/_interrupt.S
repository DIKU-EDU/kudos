/*
 * Interrupt handling
 */

#include "kernel/mips32/asm.h"

        .text
  .align  2

# interrupt_state_t _interrupt_disable(void);
  .globl  _interrupt_disable
  .ent  _interrupt_disable

_interrupt_disable:
  mfc0  v0, Status, 0
  li  t0, 0xffff00fe
  and     t1, t0, v0
  mtc0  t1, Status, 0
  j ra
        .end    _interrupt_disable


# interrupt_state_t _interrupt_enable(void);
  .globl  _interrupt_enable
  .ent  _interrupt_enable

_interrupt_enable:
  mfc0  v0, Status, 0
  li  t0, 0x0000ff01
  or      t1, t0, v0
  mtc0  t1, Status, 0
  j ra
        .end    _interrupt_enable


# interrupt_state_t interrupt_set_state(interrupt_state_t)
  .globl  _interrupt_set_state
  .ent  _interrupt_set_state

_interrupt_set_state:
  mfc0  v0, Status, 0
  li  t0, 0x0000ff01
  and  t1, t0, a0
  not     t2, t0
  and  t3, v0, t2
  or  t4, t1, t3
  mtc0  t4, Status, 0
  j ra
        .end    _interrupt_set_state

# interrupt_state_t interrupt_get_state(void)
  .globl  _interrupt_get_state
  .ent  _interrupt_get_state

_interrupt_get_state:
  mfc0  v0, Status, 0
  j ra
        .end    _interrupt_get_state

# void _interrupt_yield(void);

  .globl  _interrupt_yield
  .ent  _interrupt_yield

_interrupt_yield:
  mfc0  t0, Cause, 0
  li  t1, 0x00000100
  or      t2, t0, t1
  mtc0  t2, Cause, 0
  j ra
        .end    _interrupt_yield

# void _interrupt_clear_sw(void);

  .globl  _interrupt_clear_sw
  .ent  _interrupt_clear_sw

_interrupt_clear_sw:
  mfc0  t0, Cause, 0
  li  t1, 0xfffffcff
  and     t2, t0, t1
  mtc0  t2, Cause, 0
  j ra
        .end    _interrupt_clear_sw


# void _interrupt_clear_sw0(void);

  .globl  _interrupt_clear_sw0
  .ent  _interrupt_clear_sw0

_interrupt_clear_sw0:
  mfc0  t0, Cause, 0
  li  t1, 0xfffffeff
  and     t2, t0, t1
  mtc0  t2, Cause, 0
  j ra
        .end    _interrupt_clear_sw0

# void _interrupt_clear_sw1(void);

  .globl  _interrupt_clear_sw1
  .ent  _interrupt_clear_sw1

_interrupt_clear_sw1:
  mfc0  t0, Cause, 0
  li  t1, 0xfffffdff
  and     t2, t0, t1
  mtc0  t2, Cause, 0
  j ra
        .end    _interrupt_clear_sw1


# void _interrupt_clear_bootstrap(void);

  .globl  _interrupt_clear_bootstrap
  .ent  _interrupt_clear_bootstrap

_interrupt_clear_bootstrap:
  mfc0  t0, Status, 0
  li  t1, 0xffbffffb
  and     t2, t0, t1
  mtc0  t2, Status, 0
  j ra
        .end    _interrupt_clear_bootstrap

# /* Returns the CPU on which the calling code is running*/
# int _interrupt_getcpu(void);

  .globl  _interrupt_getcpu
  .ent  _interrupt_getcpu

_interrupt_getcpu:
  _FETCH_CPU_NUM(v0)
  j ra
        .end    _interrupt_getcpu

# void _interrupt_clear_EXL(void);

  .globl  _interrupt_clear_EXL
  .ent  _interrupt_clear_EXL

_interrupt_clear_EXL:
  mfc0  t0, Status, 0
  li  t1, 0xfffffffd
  and     t2, t0, t1
  mtc0  t2, Status, 0
  j ra
        .end    _interrupt_clear_EXL

# void _interrupt_set_EXL(void);

  .globl  _interrupt_set_EXL
  .ent  _interrupt_set_EXL

_interrupt_set_EXL:
  mfc0  t0, Status, 0
  li  t1, 0x00000002
  or      t2, t0, t1
  mtc0  t2, Status, 0
  j ra
        .end    _interrupt_set_EXL
