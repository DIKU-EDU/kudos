/*
 * Exception Handling
 */

#include <exception.h>
#include <gdt.h>
#include <idt.h>
#include "lib/types.h"
#include "kernel/interrupt.h"

void exception_init() 
{
  //Hardware exceptions x86
  idt_install_gate(0, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler0);
  idt_install_gate(1, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler1);
  idt_install_gate(2, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler2);
  idt_install_gate(3, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler3);
  idt_install_gate(4, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler4);
  idt_install_gate(5, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler5);
  idt_install_gate(6, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler6);
  idt_install_gate(7, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler7);
  idt_install_gate(8, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler8);
  idt_install_gate(9, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler9);
  idt_install_gate(10, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler10);
  idt_install_gate(11, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler11);
  idt_install_gate(12, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler12);
  idt_install_gate(13, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler13);
  idt_install_gate(14, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler14);
  idt_install_gate(15, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler15);
  idt_install_gate(16, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler16);
  idt_install_gate(17, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler17);
  idt_install_gate(18, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler18);
  idt_install_gate(19, (uint16_t)(IDT_DESC_PRESENT | IDT_DESC_BIT32), (GDT_KERNEL_CODE << 3), (irq_handler)isr_handler19);
}
