#include "proc/syscall.h"
#include "kernel/mips32/asm.h"

        .text
  .align  2
  .globl  _start
  .ent  _start

_start:
  lw  $4,4(sp) /* Copy argc from stack */
  lw  $5,0(sp) /* Copy argv from stack */
  jal  main             # call main()
  nop
  addu    a1, v0, zero     # and call syscall exit with
  li      a0, SYSCALL_EXIT # the return value
  syscall
  .end    _start

  .globl  __main
  .ent    __main

/* GCC calls __main() in the beginning of main() for C++ global variable
   initialization. Some GCC versions require this function to exist
   also for C programs. */
__main:
  jr      ra
  nop
  .end    __main
