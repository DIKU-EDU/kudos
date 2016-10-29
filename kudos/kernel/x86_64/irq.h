/*
 * Internal interrupt rutines (arch-specific)
 */

#ifndef KUDOS_KERNEL_X86_64_IRQ_H
#define KUDOS_KERNEL_X86_64_IRQ_H

#include "lib/types.h"

/* data types */
typedef uint32_t interrupt_status_t;

typedef struct regs_t
{
  /* General Registers */
  uint64_t rax, rcx, rdx, rbx;
  uint64_t kernel_rsp, rbp, rsi, rdi;
        
  /* Extra registers */
  uint64_t r8, r9, r10, r11;
  uint64_t r12, r13, r14, r15;

  /* Interrupt number & errorcode */
  uint64_t irq, errorcode;

  /* Callee information */
  uint64_t rip, cs, rflags, rsp, ss;

} regs_t;

/* Interrupt type */
typedef void (*int_handler_t)(void);

#define EFLAGS_INTERRUPT_FLAG (1 << 9)

/* Interrupts */
void isr_handler0(void);
void isr_handler1(void);
void isr_handler2(void);
void isr_handler3(void);
void isr_handler4(void);
void isr_handler5(void);
void isr_handler6(void);
void isr_handler7(void);
void isr_handler8(void);
void isr_handler9(void);
void isr_handler10(void);
void isr_handler11(void);
void isr_handler12(void);
void isr_handler13(void);
void isr_handler14(void);
void isr_handler15(void);
void isr_handler16(void);
void isr_handler17(void);
void isr_handler18(void);
void isr_handler19(void);

#endif // KUDOS_KERNEL_X86_64_IRQ_H
