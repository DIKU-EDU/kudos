#include "kernel/mips32/asm.h"

        .text
  .align  2
  .globl  _syscall
  .ent  _syscall

/* This is just a wrapper for the syscall instruction. (You can't
 * syscall in C ;). */
_syscall:
  /* All the arguments are already in place ... */
  syscall
  /* ... and the return value is already in v0. */
  jr      ra
  .end    _syscall
