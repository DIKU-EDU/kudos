/*
 * Pagetable.
 */

#ifndef KUDOS_VM_X86_64_PAGETABLE_H
#define KUDOS_VM_X86_64_PAGETABLE_H

#include "lib/types.h"

/* Number of mapping entries in one pagetable. This is the number
   of entries that fits on a single hardware memory page (4k). */
#define PAGE_TABLE_ENTRIES 512

//Limit for kernel virtual address space
#define VMM_KERNEL_SPACE 0x00007FFFFFFFFFFF

/* Page/Table/Ptr Attributes */
#define PAGE_MASK       0xFFFFFFFFFFFFF000
#define PAGE_ATTRIBS    0x0000000000000FFF
#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4
#define PAGE_WRITETHR   0x8
#define PAGE_NOT_CACHE  0x10
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40
#define PAGE_2MB        0x80
#define PAGE_CPU_GLOBAL 0x100
#define PAGE_LV4_GLOBAL 0x200

/* In x86_64, with 4 KB Pages we have 4 Level Page Directory */

/* The lowest level, is a page, which contains a physical address */
typedef uint64_t page_t;

/* The next level, is a page table, it has 512 page entries */
typedef struct page_table
{
  /* 512 Pages */
  page_t pages[PAGE_TABLE_ENTRIES];

} pagetable_t;

/* The next level is Page Directories, they have 512 page tables each */
typedef struct page_directory
{
  /* 512 Page tables
   * Physical address of each page table is contained here */
  uint64_t p_tables[PAGE_TABLE_ENTRIES];

  /* Their virtual mapping
   * this is not seen by the MMU, it is for our sake so we can
   * access them */
  uint64_t v_tables[PAGE_TABLE_ENTRIES];

} pagedirectory_t;

/* At the next leve we have Page Directory Table, it has 512 PageDirectory entries */
typedef struct page_directory_table
{
  /* 512 Page tables
   * Physical address of each page table is contained here */
  uint64_t p_page_dirs[PAGE_TABLE_ENTRIES];

  /* Their virtual mapping
   * this is not seen by the MMU, it is for our sake so we can
   * access them */
  uint64_t v_page_dirs[PAGE_TABLE_ENTRIES];

} pdp_t;

/* The top level, the PML4, it has 512 entries of PageDirectoryTables
 * Total, this gives us access of 256 Terabytes of memory, since the
 * physical address space is limited at 48 bits and not actually 64 bit */
typedef struct pml4
{
  /* 512 Page tables
   * Physical address of each page table is contained here */
  uint64_t p_page_dir_pts[PAGE_TABLE_ENTRIES];

  /* Their virtual mapping
   * this is not seen by the MMU, it is for our sake so we can
   * access them */
  uint64_t v_page_dir_pts[PAGE_TABLE_ENTRIES];

} pml4_t;

#endif // KUDOS_VM_X86_64_PAGETABLE_H
