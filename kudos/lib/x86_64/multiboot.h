/*
 * Multiboot Structure.
 */

#ifndef KUDOS_LIB_X86_64_MULTIBOOT_H
#define KUDOS_LIB_X86_64_MULTIBOOT_H

/* Includes */
#include "lib/types.h"

/* Multiboot Information Struture */
typedef struct multiboot_info
{
  uint32_t flags;
  uint32_t memory_low;
  uint32_t memory_high;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t module_count;
  uint32_t module_addr;
  union
  {
    struct
    {
      /* (a.out) Kernel Symbol table info */
      uint32_t tab_size;
      uint32_t str_size;
      uint32_t addr;
      uint32_t pad;
    } a;
    struct
    {
      /* (ELF) Kernel section header table */
      uint32_t num;
      uint32_t size;
      uint32_t addr;
      uint32_t shndx;
    } e;
  } symbols;

  /* Memory Mappings */
  uint32_t memory_map_length;
  uint32_t memory_map_addr;

  /* Drive Info */
  uint32_t drives_length;
  uint32_t drives_addr;
        
  /* ROM Configuration Table */
  uint32_t config_table;
        
  /* BootLoader Name */
  uint32_t bootloader_name;

  /* APM Table */
  uint32_t apm_table;

  /* Video Info */
  uint32_t vbe_controller_info;
  uint32_t vbe_mode_info;
  uint32_t vbe_mode;
  uint32_t vbe_interface_segment;
  uint32_t vbe_interface_offset;
  uint32_t vbe_interface_length;

} __attribute__((packed)) multiboot_info_t;

/* Flags */
#define MB_INFO_MEMORY                  0x1
#define MB_INFO_BOOTDEVICE              0x2
#define MB_INFO_CMDLINE                 0x4
#define MB_INFO_MODULES                 0x8

/* The next two are mutually exclusive */
#define MB_INFO_AOUT                    0x10
#define MB_INFO_ELF                     0x20

/* More Symbols */
#define MB_INFO_MEM_MAP                 0x40
#define MB_INFO_DRIVE_INFO              0x80
#define MB_INFO_CONFIG_TABLE            0x100
#define MB_INFO_BOOT_LDR_NAME           0x200
#define MB_INFO_APM_TABLE               0x400
#define MB_INFO_VIDEO_INFO              0x800

/* RAX must contain this */
#define MULTIBOOT_MAGIC                 0x2BADBOO2

#endif // KUDOS_LIB_X86_64_MULTIBOOT_H
