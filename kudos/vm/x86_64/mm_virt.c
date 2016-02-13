/*
 * Virtual Memory Manager for KUDOS
 */

#include "vm/memory.h"
#include "lib/libc.h"
#include "kernel/spinlock.h"
#include "kernel/interrupt.h"

//9 bit per, 12 for page

//PML Index is bits 48..39
#define VMM_INDEX_PML4(x) (((x) >> 39) & 0x1FF)

//PDP Index is bits 39..30
#define VMM_INDEX_PDP(x) (((x) >> 30) & 0x1FF)

//PD Index is bits 30..21
#define VMM_INDEX_PDIR(x) (((x) >> 21) & 0x1FF)

//PT Index is bits 21..12
#define VMM_INDEX_PTABLE(x) (((x) >> 12) & 0x1FF)

//Page mask
#define VMM_PAGE_MASK 0xFFFFFFFFFFFFF000

//Heap
#define MM_HEAP_LOCATION 0x10000000
#define MM_HEAP_END 0x20000000

/* Globals */
static pml4_t *kernel_pml4;
static uint64_t kernel_pdbr;
static spinlock_t vm_lock;

/* Helpers */
void vmm_cleartable(pagetable_t *p_table)
{
  /* Null out page table */
  if(p_table)
    memoryset(p_table, 0, sizeof(pagetable_t));
}

void vmm_cleardirectory(pagedirectory_t *p_dir)
{
  /* Null out page directory */
  if(p_dir)
    memoryset(p_dir, 0, sizeof(pagedirectory_t));
}

void vmm_clearpdp(pdp_t *p_pdp)
{
  /* Null out page directory pointer */
  if(p_pdp)
    memoryset(p_pdp, 0, sizeof(pdp_t));
}

void vmm_clearpml4(pml4_t *pml4)
{
  /* Null out pml4 */
  if(pml4)
    memoryset(pml4, 0, sizeof(pml4_t));
}

void vmm_setattribute(page_t *p, uint64_t attrib)
{
  *p |= attrib;
}

void vmm_setframe(page_t *p, uint64_t frame)
{
  *p = (*p & ~PAGE_MASK) | frame;
}

uint64_t vmm_get_kernel_pml4()
{
  return kernel_pdbr;
}

pml4_t *vmm_get_kernel_vmem()
{
  return kernel_pml4;
}

/* More helpers */
void vmm_install_ptable(pagedirectory_t *pdir, uint64_t pt_index,
                        uint64_t phys, uint64_t flags)
{
  /* Set page table attributes */
  /* We only set attributes for the physical entry
   * as the MMU will ignore the virtual one */
  uint64_t ptable = phys | flags;

  /* Now we modify the page directory entry */
  pdir->p_tables[pt_index] = ptable;
  pdir->v_tables[pt_index] = phys;

}

void vmm_install_pdir(pdp_t *pdp, uint64_t pd_index,
                      uint64_t phys, uint64_t flags)
{
  /* Set page directory attributes */
  /* We only set attributes for the physical entry
   * as the MMU will ignore the virtual one */
  uint64_t pdir = phys | flags;

  /* Now we modify the page directory pointer entry */
  pdp->p_page_dirs[pd_index] = pdir;
  pdp->v_page_dirs[pd_index] = phys;
}

void vmm_install_pdp(pml4_t *pml, uint64_t pm_index,
                     uint64_t phys, uint64_t flags)
{
  /* Set pdp attributes */
  /* We only set attributes for the physical entry
   * as the MMU will ignore the virtual one */
  uint64_t pdp = phys | flags;

  /* Now we modify the page directory pointer entry */
  pml->p_page_dir_pts[pm_index] = pdp;
  pml->v_page_dir_pts[pm_index] = phys;
}

void __attribute((noinline)) vmm_reloadcr3()
{
  /* Repoint CR3 */
  asm volatile("mov %cr3, %rax\n\t"
               "mov %rax, %cr3");
}

void vmm_setcr3(uint64_t pdbr)
{
  /* Set CR3 register to the new pdbr
   * it is physical address of pml4 */
  asm volatile("mov %%rax, %%cr3" : : "a"(pdbr));
}

void vmm_invalidatepage(uint64_t virtual_addr)
{
  asm volatile("invlpg (%%rax)" : : "a"(virtual_addr));
}

pagetable_t *vmm_getptable(pagedirectory_t *pdir, virtaddr_t vaddr)
{
  /* Get PDP index */
  uint64_t pindex = VMM_INDEX_PDIR(vaddr);
  uint64_t ptr = pdir->p_tables[pindex];
  uint64_t vptr = pdir->v_tables[pindex];

  if(ptr & PAGE_PRESENT)
    return (pagetable_t*)vptr;
  else
    return 0;
}

pagedirectory_t *vmm_getpdir(pdp_t *pdp, virtaddr_t vaddr)
{
  /* Get PDP index */
  uint64_t pindex = VMM_INDEX_PDP(vaddr);
  uint64_t ptr = pdp->p_page_dirs[pindex];
  uint64_t vptr = pdp->v_page_dirs[pindex];

  if(ptr & PAGE_PRESENT)
    return (pagedirectory_t*)vptr;
  else
    return 0;
}

pdp_t *vmm_getpdp(pml4_t *pml4, virtaddr_t vaddr)
{
  /* Get PDP index */
  uint64_t pindex = VMM_INDEX_PML4(vaddr);
  uint64_t ptr = pml4->p_page_dir_pts[pindex];
  uint64_t vptr = pml4->v_page_dir_pts[pindex];

  if(ptr & PAGE_PRESENT)
    return (pdp_t*)vptr;
  else
    return 0;
}

/* Sets up a PML4, with the first 2mb identity mapped */
void vm_init(void)
{
  /* Decls */
  uint64_t i, phys = 0, virt  = 0;
  pagetable_t *table2mb = (pagetable_t*)physmem_allocblock();
  pagetable_t *table4mb = (pagetable_t*)physmem_allocblock();
  pagedirectory_t *pdir = (pagedirectory_t*)physmem_allocblocks(2);
  pdp_t *pdp = (pdp_t*)physmem_allocblocks(2);
  kernel_pml4 = (pml4_t*)physmem_allocblocks(2);
  kernel_pdbr = (uint64_t)kernel_pml4;
  spinlock_reset(&vm_lock);

  /* Clear out allocated space */
  vmm_cleartable(table2mb);
  vmm_cleartable(table4mb);
  vmm_cleardirectory(pdir);
  vmm_clearpdp(pdp);
  vmm_clearpml4(kernel_pml4);

  /* Step 1. Identity map first 4 mb */

  /* We start at 1, because we dont want to map address 0x0,
   * this is so we can catch null-pointers */
  for(i = 1, phys = 0x1000, virt = 0x1000;
      i < 512;
      i++, phys += 0x1000, virt += 0x1000)
    {
      /* Create a page */
      /* Page structure is */
      /* Frame is the high 20 bit, which is the mapped physical
       * and the lower 12 bits are page attributes */
      page_t Page = phys | PAGE_PRESENT;

      /* Map it in */
      table2mb->pages[i] = Page;
    }

  for(i = 0, phys = 0x200000, virt = 0x200000;
      i < 512;
      i++, phys += 0x1000, virt += 0x1000)
    {
      /* Create a page */
      /* Page structure is */
      /* Frame is the high 20 bit, which is the mapped physical
       * and the lower 12 bits are page attributes */
      page_t Page = phys | PAGE_PRESENT | PAGE_WRITE;

      /* Map it in */
      table4mb->pages[i] = Page;
    }

  /* Now we setup the page directory */
  vmm_install_ptable(pdir, 0, (physaddr_t)table2mb, PAGE_PRESENT | PAGE_WRITE);
  vmm_install_ptable(pdir, 1, (physaddr_t)table4mb, PAGE_PRESENT | PAGE_WRITE);

  /* Now that we have mapped the page table into the
   * page directory, we need to map the page directory into
   * the pdp */
  vmm_install_pdir(pdp, 0, (physaddr_t)pdir, PAGE_PRESENT | PAGE_WRITE);

  /* Finally, set it to the PML4 entry */
  vmm_install_pdp(kernel_pml4, 0, (physaddr_t)pdp, PAGE_PRESENT | PAGE_WRITE);

  /* As a last and final step to setting up VM, we want to support the
   * dynamic allocation of everything to ease the kernel usage,
   * so we premap every pagetable in the heap */
  for(i = VMM_INDEX_PDIR(MM_HEAP_LOCATION);
      i < VMM_INDEX_PDIR(MM_HEAP_END); i++)
    {
      /* Get page-table */
      uint64_t *pt_entry_phys = &pdir->p_tables[i];
      uint64_t *pt_entry_virt = &pdir->v_tables[i];

      /* Get a block of memory */
      physaddr_t block = physmem_allocblock();

      /* Setup pagetable */
      vmm_setattribute(pt_entry_phys, PAGE_PRESENT | PAGE_WRITE);
      vmm_setframe(pt_entry_phys, block);
      vmm_setframe(pt_entry_virt, block);
    }

  /* Set new paging directory */
  vmm_setcr3(kernel_pdbr);
}

/* Its a simple, however still complex (programming is
 * just a lot of fun) situation. We need to check if the
 * containg pdp, pd and pagetable is present & writable,
 * else they need to get mapped aswell, its pretty tedious,
 * but whatever */
void vm_map(pagetable_t *pagetable,
            physaddr_t physaddr, virtaddr_t vaddr, int flags)
{
  /* Get current paging structure */
  pml4_t *pm;
  pdp_t *pdp;
  pagedirectory_t *pdir;
  pagetable_t *pt;

  /* Get a lock & disable ints */
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(&vm_lock);

  /* Use external PML4? */
  if(pagetable != 0)
    pm = (pml4_t*)(virtaddr_t)pagetable;
  else
    pm = kernel_pml4;

  /* Get appropriate pdp */
  pdp = vmm_getpdp(pm, vaddr);
  if(pdp == 0)
    {
      /* Oh god, it isn't, allocate a new pdp */
      pdp = (pdp_t*)physmem_allocblock();
      physmem_allocblock();
      vmm_clearpdp(pdp);

      /* Install it */
      vmm_install_pdp(pm, VMM_INDEX_PML4(vaddr),
                      (physaddr_t)pdp, PAGE_PRESENT | PAGE_WRITE | flags);

      /* Reload */
      if(pagetable == 0)
        vmm_reloadcr3();
    }

  /* Get appropriate pdir */
  pdir = vmm_getpdir(pdp, vaddr);
  if(pdir == 0)
    {
      /* Oh god, it isn't, allocate a new page directory */
      pdir = (pagedirectory_t*)physmem_allocblock();
      physmem_allocblock();
      vmm_cleardirectory(pdir);

      /* Install it */
      vmm_install_pdir(pdp, VMM_INDEX_PDP(vaddr),
                       (physaddr_t)pdir, PAGE_PRESENT | PAGE_WRITE | flags);

      /* Reload */
      if(pagetable == 0)
        vmm_reloadcr3();
    }

  /* Get appropriate page directory */
  pt = vmm_getptable(pdir, vaddr);
  if(pt == 0)
    {
      /* Oh god, it isn't, allocate a new page table */
      pt = (pagetable_t*)physmem_allocblock();
      vmm_cleartable(pt);

      /* Install it */
      vmm_install_ptable(pdir, VMM_INDEX_PDIR(vaddr),
                         (physaddr_t)pt, PAGE_PRESENT | PAGE_WRITE | flags);

      /* Reload */
      if(pagetable == 0)
        vmm_reloadcr3();
    }

  /* NOW, FINALLY, Get the appropriate page */
  pt->pages[VMM_INDEX_PTABLE(vaddr)] = physaddr |
    PAGE_PRESENT | PAGE_WRITE | flags;

  /* Done, release lock */
  spinlock_release(&vm_lock);
  _interrupt_set_state(intr_status);

  /* Invalidate page in TLB cache */
  if(pagetable == 0)
    vmm_invalidatepage(vaddr);
}

void vm_unmap(pagetable_t *pagetable, virtaddr_t vaddr)
{
  /* Unimplemented */
  vaddr = vaddr;
}

physaddr_t vm_getmap(pagetable_t *pagetable, virtaddr_t vaddr)
{
  /* Get current paging structure */
  pml4_t *pm;
  pdp_t *pdp;
  pagedirectory_t *pdir;
  pagetable_t *pt;
  page_t *page;
  uint32_t pindex = 0;

  /* Get a lock & disable ints */
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(&vm_lock);

  /* Use external PML4? */
  if(pagetable != 0)
    pm = (pml4_t*)(virtaddr_t)pagetable;
  else
    pm = kernel_pml4;

  /* Get appropriate pdp */
  pdp = vmm_getpdp(pm, vaddr);
  if(pdp == 0)
    return 0;

  /* Get appropriate pdir */
  pdir = vmm_getpdir(pdp, vaddr);
  if(pdir == 0)
    return 0;

  /* Get appropriate page directory */
  pt = vmm_getptable(pdir, vaddr);
  if(pt == 0)
    return 0;

  /* NOW, FINALLY, Get the appropriate page */
  pindex = VMM_INDEX_PTABLE(vaddr);
  page = &pt->pages[pindex];

  /* Done, release lock */
  spinlock_release(&vm_lock);
  _interrupt_set_state(intr_status);

  /* Return frame */
  return (physaddr_t)((physaddr_t)page & 0xFFFFFFFFFFFFF000);
}

pagetable_t *vm_create_pagetable(uint32_t asid)
{
  /* Ok, this function creates a new page_table */
  asid = asid;
  pdp_t *pdp = (pdp_t*)physmem_allocblocks(2);
  pml4_t *pml4 = (pml4_t*)physmem_allocblocks(2);
  pdp_t *kpdp = 0;
  uint64_t pdbr = (uint64_t)pml4;
  pdbr = pdbr;

  /* We need the first 4 mb identity mapped aswell, like
   * kernel directory */
  vmm_clearpdp(pdp);
  vmm_clearpml4(pml4);

  /* Transfer the first pdir */
  kpdp = vmm_getpdp(kernel_pml4, 0);
  if(kpdp == 0)
    {
      kprintf("vm_create: error pdp!!\n");
      for(;;);
    }

  /* Make the transfer! */
  /* Install pdp and pdir */
  vmm_install_pdp(pml4, 0, (physaddr_t)pdp, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
  vmm_install_pdir(pdp, 0, kpdp->v_page_dirs[0], PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

  return (pagetable_t*)(uint64_t)pml4;
}

/**
 * Destroys given pagetable. Frees the memory allocated for
 * the pagetable.
 *
 * @param pagetable Page table to destroy
 *
 */
void vm_destroy_pagetable(pagetable_t *pagetable)
{
  pagetable=pagetable;
  /* FIXME - we need to implement this! */
}

/* Compatability Functions */
uintptr_t _tlb_get_maxindex(void)
{
  return 512;
}
