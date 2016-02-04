/*
 * The interrupt descriptor table
 */
 
#include <gdt.h>
#include <idt.h>
#include "lib/libc.h"

/* Default Interrupt Handler */
extern void isr_default_handler();

/* IDT */
static idt_desc_t idt_descriptors[MAX_INTERRUPTS];
static idt_t idt_table;

void idt_init()
{
  /* Decls */
  int64_t i;

  /* Install Dummy Descriptors */
  for(i = 0; i < MAX_INTERRUPTS; i++)
    {
      idt_install_gate((uint32_t)i, IDT_DESC_PRESENT | IDT_DESC_BIT32,
                       (GDT_KERNEL_CODE << 3), (irq_handler)isr_default_handler);
    }

  /* Setup table */
  idt_table.Limit = (uint16_t)(sizeof(idt_desc_t) * MAX_INTERRUPTS) - 1;
  idt_table.Base = (uint64_t)&idt_descriptors;

  /* Install table */
  asm volatile("lidt (%%rax)" : : "a"((uint64_t)&idt_table));
}

void idt_install_gate(uint32_t index, uint16_t flags, 
                      uint16_t selector, irq_handler Irq)
{
  /* Sanity */
  if(index > MAX_INTERRUPTS)
    return;

  if(!Irq)
    return;

  /* Get base of interrupt handler */
  uint64_t irq_base = (uint64_t)&(*Irq);

  /* Create IDT Entry */
  idt_descriptors[index].base_low = (uint16_t)(irq_base & 0xFFFF);
  idt_descriptors[index].base_mid = (uint16_t)((irq_base >> 16) & 0xFFFF);
  idt_descriptors[index].base_high = (uint32_t)((irq_base >> 32) & 0xFFFFFFFF);
  idt_descriptors[index].reserved0 = 0;
  idt_descriptors[index].reserved1 = 0;
  idt_descriptors[index].flags = (uint8_t)flags;
  idt_descriptors[index].selector = selector;
}
