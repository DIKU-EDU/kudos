/*
 * Context switch code.
 */

#include "kernel/mips32/asm.h"
#include "kernel/config.h"

        .text
  .align  2
        # Do not let the assembler to reorder the code or use macros,
        # which might use undesirable registers
        .set noreorder
        .set nomacro

        # The code to be inserted to the interrupt vector. Contains only
        # a jump to the context switch code. Must be _exactly_ 8 words.
        .globl  _cswitch_vector_code
  .ent  _cswitch_vector_code
_cswitch_vector_code:
        j       _cswitch_switch
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        .end    _cswitch_vector_code

        # The context switch code
  .globl _cswitch_switch
  .ent    _cswitch_switch
_cswitch_switch:
  # Check if this exception occured in user mode:
  mfc0  k0, Status, 0
  andi  k0, k0, 0x10
  beqz  k0, _not_usermode_exception  # branch on kernel mode
  nop

  # This is a safe macro instruction
        .set    macro
        la      k0, scheduler_current_thread
        .set    nomacro

  _FETCH_CPU_NUM(k1)

  # get TID from table indexed by processor number
  sll  k1, k1, 2    # size of entries in scheduler_current_thread
                             # is 4
  addu  k0, k0, k1   # Get address of this CPUs current thread
        lw      k0, 0(k0)    # ...and load the TID.
        sll     k0, k0, 6    # TID*64, offset from beginning of thread table

        # Again a safe macro.
  .set  macro
        la      k1, thread_table
        .set    nomacro

        addu    k1, k0, k1        # address of thread strucure
  lw  k1, 0(k1)    # load old context pointer
  nop
  lw  k0, 104(k1)    # load top of kernel stack (saved sp)
  j       _usermode_exception
  nop

_not_usermode_exception:
  # Save context to kernel stack of the thread
  addu    k0, sp, zero

_usermode_exception:
  addiu  k0, k0, -136      # make room for context in stack
        # Safe macro
  .set    macro
        la      k1, _cswitch_r1   # load return address for context save
        .set    nomacro

        j  _cswitch_context_save
        nop
_cswitch_r1:
        # Now context is saved and we may use all registers
  # Set sp to point to current point in kernel stack
  addu  sp, k0, zero

  # Set variables in thread structure
  # This is a safe macro, but it does not matter at this point anymore
        .set    macro
        la      k0, scheduler_current_thread
        .set    nomacro

  _FETCH_CPU_NUM(k1)

  # get TID from table indexed by processor number
  sll  k1, k1, 2    # size of entries in scheduler_current_thread
                             # is 4
  addu  k0, k0, k1   # Get address of this CPUs current thread
        lw      k0, 0(k0)    # ...and load the TID.
        sll     k0, k0, 6    # TID*64, offset from beginning of thread table

        # Again a safe macro.
  .set  macro
        la      k1, thread_table
        .set    nomacro

        addu    t1, k0, k1        # address of thread strucure
  lw  t0, 0(t1)    # load old context pointer
  nop
  sw  t0, 132(sp)    # save old context pointer

  beqz    k0, _cswitch_in_idle_thread # ignore context of idle thread
  nop
  sw  sp, 0(t1)    # set new context pointer in thread
_cswitch_in_idle_thread:
  # Thread structure variables are now set


  # Setup stack for interrupt/exception handling C-code
  # Check wheter we are in interrupt
  mfc0  k1, Cause, 0
  srl  k1, k1, 2
  andi  k1, k1, 0x1f
  bnez  k1, _cswitch_stack_ok # not in interrupt
  nop

  # We come here because of an interrupt: use interrupt stack
        # safe macro
  .set    macro
        la      k0, interrupt_stacks
        .set    nomacro
  _FETCH_CPU_NUM(k1)

  # set up interrupt stack in k0
  # get SP from table indexed by processor number
  sll  k1, k1, 2    # Element size is 4
  addu  k0, k0, k1   # Get address of this CPUs interrupt stack address
  lw      sp, 0(k0)    # Get stack address
_cswitch_stack_ok:
  # Subtract one word from SP to follow GCC calling conventions
        # (Space for argument must be reserved in stack even when
  # the argument is passed in a register.)
  addu  sp, sp, -4

  # Stack is now set up for C-code

  # Clear User Mode bit (it is enabled if we come from userland)

  mfc0  k0, Status, 0
  .set macro
  li      k1, 0xffffffef
  li  t1, 0x00000010
  .set nomacro
  and     t7, t1, k0 # Save UM bit for later use
  and  k0, k0, k1 # And clear it
  mtc0    k0, Status, 0 # After this Alice will be in wonderland

  # Exception code as parameter
  mfc0  a0, Cause, 0
  srl  a0, a0, 2
  andi  a0, a0, 0x1f
  beqz  a0, _intr_handle
  nop

  # Check if this exception occured in user mode:
  beqz  t7, _kexc_handle  # exception from kernel mode
  nop
  j  _uexc_handle      # exception from user mode
  nop

_intr_handle:
  # Reload Cause as parameter
  mfc0  a0, Cause, 0
  jal  interrupt_handle  # jump to c code
  nop
  j  _handle_finished
  nop
_kexc_handle:
  jal  kernel_exception_handle  # jump to c code
  nop
  j  _handle_finished
  nop
_uexc_handle:
  jal  user_exception_handle  # call c code handler
  nop

_handle_finished:

  # restore context
        .set    macro
        la      k0, scheduler_current_thread
        .set    nomacro

        #fetch cpu number...
  _FETCH_CPU_NUM(k1)

  # get TID from table indexed by processor number
        #(same as before context save)
  sll  k1, k1, 2
  addu  k0, k0, k1
        lw      k0, 0(k0)
        nop
        sll     k0, k0, 6       # TID*64, offset from beginning of table
  .set  macro
        la      k1, thread_table
        .set    nomacro
        addu    t0, k0, k1      # address of thread structure

  lw  k0, 0(t0)  # load context structure address
  nop
  lw  t1, 132(k0)  # load old context pointer
  nop
  sw  t1, 0(t0)  # restore old context pointer

  .set    macro
        la      k1, _cswitch_r2 # load return address for context_restore
        .set    nomacro

        j  _cswitch_context_restore
  nop

_cswitch_r2:
        # return to the context of the thread
  eret
  nop
        .end    _cswitch_switch


  # Save context. Address of context in register k0 and
  # return address in k1.
        .ent    _cswitch_context_save
_cswitch_context_save:
        # save registers

        # We want to use at, no matter what the assembler wants to do
        .set    noat
        sw      AT, 0(k0)
        .set    at
        sw      v0, 4(k0)
        sw      v1, 8(k0)
        sw      a0, 12(k0)
        sw      a1, 16(k0)
        sw      a2, 20(k0)
        sw      a3, 24(k0)
        sw      t0, 28(k0)
        sw      t1, 32(k0)
        sw      t2, 36(k0)
        sw      t3, 40(k0)
        sw      t4, 44(k0)
        sw      t5, 48(k0)
        sw      t6, 52(k0)
        sw      t7, 56(k0)
        sw      s0, 60(k0)
        sw      s1, 64(k0)
        sw      s2, 68(k0)
        sw      s3, 72(k0)
        sw      s4, 76(k0)
        sw      s5, 80(k0)
        sw      s6, 84(k0)
        sw      s7, 88(k0)
        sw      t8, 92(k0)
        sw      t9, 96(k0)
        sw      gp, 100(k0)
        sw      sp, 104(k0)
        sw      fp, 108(k0)
        sw      ra, 112(k0)
        mfhi    t1              # save hi register
        sw      t1, 116(k0)
        mflo    t1              # save lo register
        sw      t1, 120(k0)
        mfc0    t1, EPC, 0      # EPC
        sw      t1, 124(k0)
        mfc0    t1, Status, 0
        andi    t1, t1, 0xff11  # save only interrupt mask and UserMode bits
        sw      t1, 128(k0)
  jr  k1
  nop
        .end    _cswitch_context_save

  # Restore context. Address of context in register k0 and
  # return address in k1.
        .ent    _cswitch_context_restore
_cswitch_context_restore:
        lw      t1, 116(k0)
  nop
        mthi    t1              # restore hi register
        lw      t1, 120(k0)
  nop
  mtlo    t1              # restore lo register
        lw      t1, 124(k0)
  nop
        mtc0    t1, EPC, 0      # EPC
        lw      t1, 128(k0)
  nop
  mfc0    t2, Status, 0
  .set macro
  li      t3, 0xffff00ee
  .set nomacro
  and  t2, t2, t3
  or      t1, t2, t1
        mtc0    t1, Status, 0

        # restore registers

        # do not let the assembler whine about using the at register
        .set    noat
        lw      AT, 0(k0)
        .set    at
        lw      v0, 4(k0)
        lw      v1, 8(k0)
        lw      a0, 12(k0)
        lw      a1, 16(k0)
        lw      a2, 20(k0)
        lw      a3, 24(k0)
        lw      t0, 28(k0)
        lw      t1, 32(k0)
        lw      t2, 36(k0)
        lw      t3, 40(k0)
        lw      t4, 44(k0)
        lw      t5, 48(k0)
        lw      t6, 52(k0)
        lw      t7, 56(k0)
        lw      s0, 60(k0)
        lw      s1, 64(k0)
        lw      s2, 68(k0)
        lw      s3, 72(k0)
        lw      s4, 76(k0)
        lw      s5, 80(k0)
        lw      s6, 84(k0)
        lw      s7, 88(k0)
        lw      t8, 92(k0)
        lw      t9, 96(k0)
        lw      gp, 100(k0)
        lw      sp, 104(k0)
        lw      fp, 108(k0)
        lw      ra, 112(k0)
  jr  k1
  nop

        .end    _cswitch_context_restore

# void _cswitch_to_userland(context_t *usercontext)
#
# Switches to userland
        .globl  _cswitch_to_userland
  .ent  _cswitch_to_userland
  _cswitch_to_userland:

  # Set EXL
  mfc0  t0, Status, 0
  .set macro
  li      t1, 0x00000002
  .set nomacro
  or      t0, t0, t1
  mtc0    t0, Status, 0

  addu    k0, a0, zero # Copy usercontext pointer to k0

  .set    macro
        la      k1, _cswitch_r4 # load return address for context_restore
        .set    nomacro

        j  _cswitch_context_restore
  nop

_cswitch_r4:
        # Jump to userland
  eret

  .end _cswitch_to_userland

        .set    reorder
        .set    macro

