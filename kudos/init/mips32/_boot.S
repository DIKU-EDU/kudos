/*
 * The boot assembler code (linker entry point is _start).
 */

#include "kernel/mips32/asm.h"

  .text
  .align  2
  .globl  _start
  .ent  _start

_start:
  # If we are running on a CPU other than 0, wait
  # until CPU 0 has initialized the system.
  _FETCH_CPU_NUM(t0)
  bnez  t0, init_wait_loop

  # Only CPU 0 runs these
  li  sp, 0x8000fffc        # Initialize SP
  j init                    # Jump to init() in main.c
  nop
  .end    _start


  .ent init_wait_loop
init_wait_loop:
  # Wait until kernel_bootstrap_finished != 0
  la  t0, kernel_bootstrap_finished
init_wait_more:
  lw  t1, (t0)
  beqz  t1, init_wait_more

  li      sp, 0x80007ffc    # Initialize SP

  # Exit bootstrap mode and enable interrupts
  jal _interrupt_clear_bootstrap
  jal _interrupt_enable

  # Force a context switch and scheduling
  jal _interrupt_yield

  j _idle_thread_wait_loop
  .end init_wait_loop
