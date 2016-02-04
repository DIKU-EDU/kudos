/*
 * The global descriptor table
 */

#include <gdt.h>
#include "lib/libc.h"

/* GDT */
static gdt_desc_t gdtdescriptors[MAX_DESCRIPTORS];
static gdt_t gdt;
static uint32_t gdt_index;

void gdt_init()
{
  /* null out the gdt & descriptors */
  gdt_index = 0;
  memoryset(&gdt, 0, sizeof(gdt_t));
  memoryset(&gdtdescriptors, 0, sizeof(gdt_desc_t) * MAX_DESCRIPTORS);

  /* Setup table */
  gdt.limit = (sizeof(gdt_desc_t) * MAX_DESCRIPTORS) - 1;
  gdt.base = (uint64_t)&gdtdescriptors[0];

  /* Install Descriptors */

  /* Null descriptor, all GDT starts with null descriptors */
  gdt_install_descriptor(0, 0, 0, 0);

  /* Kernel Code Descriptor, 0x08 */
  gdt_install_descriptor(0, 0,
                         GDT_DESC_MEMORY | GDT_DESC_READWRITE |
                         GDT_DESC_EXECUTABLE | GDT_DESC_CODEDATA,
                         GDT_GRAN_4K | GDT_GRAN_64BIT);

  /* Kernel Data Descriptor, 0x10 */
  gdt_install_descriptor(0, 0,
                         GDT_DESC_MEMORY | GDT_DESC_READWRITE |
                         GDT_DESC_CODEDATA, GDT_GRAN_4K | GDT_GRAN_64BIT);

  /* User Code Descriptor, 0x18 */
  gdt_install_descriptor(0, 0,
                         GDT_DESC_MEMORY | GDT_DESC_READWRITE |
                         GDT_DESC_EXECUTABLE | GDT_DESC_CODEDATA | GDT_DESC_DPL,
                         GDT_GRAN_4K | GDT_GRAN_64BIT);

  /* User Data Descriptor, 0x20 */
  gdt_install_descriptor(0, 0,
                         GDT_DESC_MEMORY | GDT_DESC_READWRITE |
                         GDT_DESC_CODEDATA | GDT_DESC_DPL,
                         GDT_GRAN_4K | GDT_GRAN_64BIT);

  /* Install table */
  asm volatile("lgdt (%%rax)" : : "a"((uint64_t)&gdt));
}

void gdt_install_descriptor(uint64_t base, uint64_t limit,
                            uint8_t access, uint8_t granularity)
{
  /* Sanity */
  if(gdt_index > MAX_DESCRIPTORS)
    return;

  /* Setup */
  gdtdescriptors[gdt_index].base_low = (uint16_t)(base & 0xFFFF);
  gdtdescriptors[gdt_index].base_mid = (uint8_t)((base >> 16) & 0xFF);
  gdtdescriptors[gdt_index].base_high = (uint8_t)((base >> 24) & 0xFF);
  gdtdescriptors[gdt_index].limit = (uint16_t)(limit & 0xFFFF);

  gdtdescriptors[gdt_index].flags = access;
  gdtdescriptors[gdt_index].granularity = (uint8_t)((limit >> 16) & 0x0F);
  gdtdescriptors[gdt_index].granularity |= granularity & 0xF0;

  gdt_index++;
}

void gdt_install_tss(uint64_t base, uint64_t limit)
{
  /* Setup */
  uint16_t tss_type = 0x0089;
  gdt_sys_desc_t *gdt_desc = (gdt_sys_desc_t*)&gdtdescriptors[gdt_index];

  /* Sanity */
  if(gdt_index > MAX_DESCRIPTORS)
    return;

  gdt_desc->type_0 = (uint16_t)(tss_type & 0x00FF);
  gdt_desc->limit_1 = (uint8_t)((tss_type & 0xF000) >> 12);

  gdt_desc->addr_0 = base & 0xFFFF;
  gdt_desc->addr_1 = (base & 0xFF0000) >> 16;
  gdt_desc->addr_2 = (base & 0xFF000000) >> 24;
  gdt_desc->addr_3 = base >> 32;

  gdt_desc->limit_0 = limit & 0xFFFF;
  gdt_desc->limit_1 = (limit & 0xF0000) >> 16;

  gdt_desc->reserved = 0;

  gdt_index += 2;
}
