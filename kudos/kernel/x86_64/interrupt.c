/*
 * Interrupt handling
 */

#include "kernel/interrupt.h"
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <tss.h>
#include <exception.h>
#include "lib/libc.h"

/* Initial stack start */
uint64_t init_stack = 0x90000;

/* Externs */
extern void syscall_init(void);
extern void yield_irq_handler(void);
extern void __enable_irq(void);
extern void __disable_irq(void);
extern uint64_t __getflags(void);

/* Holds current interrupt status */

interrupt_status_t _interrupt_disable(void)
{
  interrupt_status_t cur_state = _interrupt_get_state();
  __disable_irq();
  return cur_state;
}

interrupt_status_t _interrupt_enable(void)
{
  interrupt_status_t cur_state = _interrupt_get_state();
  __enable_irq();
  return cur_state;
}

interrupt_status_t _interrupt_set_state(interrupt_status_t stat)
{
  if(stat != 0)
    return _interrupt_enable();
  else
    return _interrupt_disable();

}

interrupt_status_t _interrupt_get_state(void)
{
  interrupt_status_t status = (interrupt_status_t)__getflags();
  if(status & EFLAGS_INTERRUPT_FLAG)
    return 1;
  else
    return 0;
}

interrupt_status_t _interrupt_is_disabled(void)
{
  return !_interrupt_get_state();
}

void interrupt_init(int num_cpus) 
{
  num_cpus = num_cpus;
        
  /* Setup descriptors */
  gdt_init();
  idt_init();

  /* Setup basic interrupts */
  exception_init();
  pic_init();
  tss_init();
  tss_install(0, init_stack);

  /* Syscall vector and YIELD vector */
  syscall_init();

  idt_install_gate(0x81, IDT_DESC_PRESENT | IDT_DESC_BIT32 | IDT_DESC_RING3, 
                   (GDT_KERNEL_CODE << 3), (int_handler_t)yield_irq_handler);
}

void interrupt_register(uint32_t irq,
                        int_handler_t handler,
                        device_t *device)
{
  device = device;
        
  /* Install interupt, first 32 interrupts are reserved */
  idt_install_gate(0x20 + irq, 
                   IDT_DESC_PRESENT | IDT_DESC_BIT32, 
                   (GDT_KERNEL_CODE << 3), 
                   handler);
}

void interrupt_handle(virtaddr_t cause)
{
  /* Get registers */
  regs_t *Registers = (regs_t*)cause;

  switch(Registers->irq)
    {
      /* Divide By Zero */
    case 0:
      {
        kprintf("Divide by Zero!\n");
      } break;
      /* Debug Trap */
    case 1:
      {
        kprintf("Debug Trap!!\n");
      } break;
      /* NMI */
    case 2:
      {
        kprintf("Non-maskable Interrupt Trap!\n");
      } break;
      /* Breakpoint Trap */
    case 3:
      {
        kprintf("Breakpoint Trap!\n");
      } break;
      /* Overflow Exception */
    case 4:
      {
        kprintf("Overflow Exception!\n");
      } break;
      /* Bound Range Exceeded */
    case 5:
      {
        kprintf("Array Bounds Exception!\n");
      } break;
      /* Invalid Opcode */
    case 6:
      {
        kprintf("Invalid Opcode Exception!\n");
      } break;
      /* Device Not Available */
    case 7:
      {
        kprintf("Device Fault!\n");
      } break;
      /* Double Fault */
    case 8:
      {
        kprintf("Double Fault!\n");
      } break;
      /* Segment Fault */
    case 9:
      {
        kprintf("Segment Error!\n");
      } break;
      /* Invalid TSS  */
    case 10:
      {
        kprintf("Invalid TSS Exception!\n");
      } break;
      /* Segment Not Present */
    case 11:
      {
        kprintf("Segment Exception!\n");
      } break;
      /* Stack Segment Fault */
    case 12:
      {
        kprintf("Stack Segment Fault!\n");
      } break;
      /* General Protection Fault */
    case 13:
      {
        kprintf("General Protection Fault!\n");
      } break;
      /* Page Fault */
    case 14:
      {
        kprintf("Page Fault!\n");
      } break;
      /* Reserved Exception */
    case 15:
      {
        kprintf("General Exception!\n");
      } break;
      /* FPU Exception */
    case 16:
      {
        kprintf("FPU Exception, don't use floats!\n");
      } break;
      /* Alignment Check */
    case 17:
      {
        kprintf("Alignment Check Exception!\n");
      } break;
      /* Machine Check */
    case 18:
      {
        kprintf("Machine Check Exception!\n");
      } break;
      /* SIMD Exception */
    case 19:
      {
        kprintf("SIMD Exception!\n");
      } break;
    }

  /* Info */
  kprintf("Exception: 0x%xl (errcode 0x%xl) Occured!\n", 
          Registers->irq, Registers->errorcode);
  kprintf("Exception occured at address 0x%xl\n", Registers->rip);

  for(;;);

  /* Send EOI */
}
