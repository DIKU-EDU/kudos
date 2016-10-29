/*
 * The global descriptor table.
 */

#ifndef KUDOS_KERNEL_X86_64_GDT_H
#define KUDOS_KERNEL_X86_64_GDT_H

/* Includes */
#include "lib/types.h"

/* Defines */
#define GDT_DESC_ACCESS                 0x1     /* Access Bit */
#define GDT_DESC_READWRITE              0x2     /* If set, we have read/write priveligies */
#define GDT_DESC_EXPANSION              0x4     /* Direction of expansion */
#define GDT_DESC_EXECUTABLE             0x8     /* Descriptor TYPE */
#define GDT_DESC_CODEDATA               0x10    /* Is this a code or data descriptor ? */
#define GDT_DESC_DPL                    0x60    /* Priveligie Level (0 - Kernel) */
#define GDT_DESC_MEMORY                 0x80    /* Set present bit */

#define GDT_GRAN_LIMITHI_MASK           0x0F    /* Masks out high-limit */
#define GDT_GRAN_OS                     0x10    /* Set OS defined bit */
#define GDT_GRAN_64BIT                  0x20    /* Set if 64 bit */
#define GDT_GRAN_32BIT                  0x40    /* Set if 32 bit, else its 16 bit */
#define GDT_GRAN_4K                     0x80    /* Set for 4K Grandularity, default is none */

/* GDT Descriptor Entries */
#define GDT_KERNEL_CODE                 0x1
#define GDT_KERNEL_DATA                 0x2
#define GDT_USER_CODE                   0x3
#define GDT_USER_DATA                   0x4


/* Structures */
#define MAX_DESCRIPTORS                 16

/* GDT Descriptor */
typedef struct gdt_descriptor
{
  /* Bits 0-15 of segment limit */
  uint16_t limit;

  /* Bits 0-23 of base address */
  uint16_t base_low;

  /* ^ 16 - 23 */
  uint8_t base_mid;

  /* Descriptor Access Flags */
  uint8_t flags;

  /* Granularity */
  uint8_t granularity;

  /* 23-31 of base address */
  uint8_t base_high;

} __attribute__((packed)) gdt_desc_t;

typedef struct gdt_system_descriptor
{
  /* Limit 1 */
  uint16_t limit_0;
  uint16_t addr_0;

  uint8_t addr_1;
  uint8_t type_0;

  /* Low 4 bits - Limit, High 4 bits - Type1 */
  uint8_t limit_1;
  uint8_t addr_2;

  uint32_t addr_3;
  uint32_t reserved;

} __attribute__((packed)) gdt_sys_desc_t;

/* Global Descriptor Table */
typedef struct gdt
{
  /* Size of GDT */
  uint16_t limit;

  /* Base Address of GDT */
  uint64_t base;

} __attribute__((packed)) gdt_t;

/* Prototypes */
void gdt_init();

void gdt_install_tss(uint64_t base, uint64_t limit);
void gdt_install_descriptor(uint64_t base, uint64_t limit,
                            uint8_t access, uint8_t grandularity);


#endif // KUDOS_KERNEL_X86_64_GDT_H
