/*
 * The interrupt descriptor table.
 */

#ifndef KUDOS_KERNEL_X86_64_IDT_H
#define KUDOS_KERNEL_X86_64_IDT_H

/* Includes */
#include "lib/types.h"

/* Defines */
#define IDT_DESC_BIT16                  0x07    //16 Bit
#define IDT_DESC_BIT32                  0x0F    //32 Bit Trap Gate
#define IDT_DESC_RING1                  0x40    //Priveligie Level 1
#define IDT_DESC_RING2                  0x20    //Priveligie Level 2
#define IDT_DESC_RING3                  0x60    //Priveligie Level 3
#define IDT_DESC_PRESENT                0x80    //Is it present? 

/* Callback Type (Interrupt) */
typedef void (*irq_handler)();

/* Structures */
#define MAX_INTERRUPTS                  0xFF

/* IDT Descriptor */
typedef struct idt_descriptor
{
  /* Base Bits 0..15 */
  uint16_t base_low;

  /* Selector (Code Segment) */
  uint16_t selector;

  /* Reserved */
  uint8_t reserved0;

  /* Attributes */
  uint8_t flags;

  /* Base Bits 16..31 */
  uint16_t base_mid;

  /* Base Bits 32..63 */
  uint32_t base_high;

  /* Reserved */
  uint32_t reserved1;

} __attribute__((packed)) idt_desc_t;

/* Interrupt Descriptor Table */
typedef struct id_table
{
  /* Size of IDT */
  uint16_t Limit;

  /* Base Address of IDT */
  uint64_t Base;

} __attribute__((packed)) idt_t;

/* Prototypes */
void idt_init();

void idt_install_gate(uint32_t index, uint16_t flags, uint16_t selector, irq_handler Irq);

#endif // KUDOS_KERNEL_X86_64_IDT_H
