/*
 * Virtual Memory Manager for KUDOS
 */

#include "vm/memory.h"
#include "lib/libc.h"
#include "kernel/panic.h"
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

/* Extern variables */
extern uint64_t KERNEL_ENDS_HERE;   //physical address of kernel end
extern physaddr_t stalloced_total;  //Total bytes stalloced

/* The virtual address for the next stalloc allocation */
virtaddr_t kmalloc_addr;

/* Page table space of 4mb*/
pagetable_t pt_pool[VM_PTP_SIZE] __attribute__ ((aligned (4096)));
/* Bitmap of free page tables */
uint64_t pt_bitmap[VM_PTP_SIZE/64];
/* Extern functions to manipulate the pt bitmap */

/* Globals */
static pagetable_t *kernel_pml4;
static spinlock_t vm_lock;

/* Page table Bitmap Helpers */
void ptmap_setbit(int64_t bit)
{
  pt_bitmap[bit / 64] |= (1 << (bit % 64));
}

void ptmap_unsetbit(int64_t bit)
{
  pt_bitmap[bit / 64] &= ~(1 << (bit % 64));
}

int64_t ptmap_testbit(int64_t bit)
{
  return pt_bitmap[bit / 64] & (1 << (bit % 64));
}

/* Helpers */
void vmm_cleartable(pagetable_t *p_table)
{
  /* Null out page table */
  if(p_table)
    memoryset(p_table, 0, sizeof(pagetable_t));
}

void vmm_setattribute(page_t *p, uint64_t attrib)
{
  *p |= attrib;
}

void vmm_setframe(page_t *p, uint64_t frame)
{
  *p = (*p & ~PAGE_MASK) | frame;
}

pagetable_t* vmm_get_kernel_pml4(){
  return kernel_pml4;
}

pagetable_t* vmm_new_pagetable(){
  pagetable_t* pt;
  uint64_t i;
    for(i = 0; i < VM_PTP_SIZE; i++){
      if(!ptmap_testbit(i)){
        ptmap_setbit(i);
        pt = &pt_pool[i];
        break;
      }
    }
  if(i == VM_PTP_SIZE)
    KERNEL_PANIC("No more pagetables");

  return pt;
}

void vmm_install_ptable(pagetable_t *target, uint64_t pt_index,
                        uint64_t phys, uint64_t flags)
{
    /* Set page table attributes */
    uint64_t entry = phys | flags;

    /* Add the page table entry to the page directory */
    target->pages[pt_index] = entry;
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

pagetable_t* vmm_getptable(pagetable_t *pdir, virtaddr_t vaddr)
{
  /* Get PDP index */
  uint64_t pindex = VMM_INDEX_PDIR(vaddr);
  uint64_t ptr = pdir->pages[pindex];

  if(ptr & PAGE_PRESENT)
    return (pagetable_t*)(ptr & PAGE_MASK);
  else
    return 0;
}

pagetable_t* vmm_getpdir(pagetable_t *pdp, virtaddr_t vaddr)
{
  /* Get PDP index */
  uint64_t pindex = VMM_INDEX_PDP(vaddr);
  uint64_t ptr = pdp->pages[pindex];

  if(ptr & PAGE_PRESENT)
    return (pagetable_t*)(ptr & PAGE_MASK);
  else
    return 0;
}

pagetable_t* vmm_getpdp(pagetable_t *pml4, virtaddr_t vaddr)
{
  /* Get PDP index */
  uint64_t pindex = VMM_INDEX_PML4(vaddr);
  uint64_t ptr = pml4->pages[pindex];

  if(ptr & PAGE_PRESENT)
    return (pagetable_t*)(ptr & PAGE_MASK);
  else
    return 0;
}

void vm_init(void){
  uint64_t i, phys, virt;
  physaddr_t indentity_bound;
  pagetable_t *pml4;
  spinlock_reset(&vm_lock);

  /* The boundary for the indentity mapping */
  indentity_bound = ((physaddr_t)&KERNEL_ENDS_HERE)+stalloced_total;

  if (indentity_bound % PMM_BLOCK_SIZE > 0) {
    indentity_bound += PMM_BLOCK_SIZE - indentity_bound % PMM_BLOCK_SIZE;
  }

  /* initialize kmalloc allocation address */
  kmalloc_addr = indentity_bound;
    
  /* Clear page table bitmap */
  for(uint64_t i = 0; i < VM_PTP_SIZE; i++)
    ptmap_unsetbit(i);

  /* Create kernel pml4 */
  pml4 = vmm_new_pagetable();
  vmm_cleartable(pml4);

 /* Identity map from page 1 to KERNEL_ENDS_HERE */
  for(i = 1, phys = virt = 0x1000; phys < indentity_bound;
      i++, phys = virt += 0x1000)
  {
    vm_map(pml4, phys, virt, 0);
  }

  kernel_pml4 = pml4;
  vmm_setcr3((uint64_t) pml4);
}

void* kmalloc(uint64_t size){
  physaddr_t frames;
  virtaddr_t ret_addr;
  uint64_t n_frames, i;
  if(size+kmalloc_addr > VMM_KERNEL_SPACE)
    KERNEL_PANIC("kmalloc: Out of virtual address space");

  ret_addr = kmalloc_addr;

  n_frames = size/PMM_BLOCK_SIZE;
  if(size%PMM_BLOCK_SIZE)
    n_frames++;

  frames = physmem_getframes(n_frames);
  for(i = 0; i < n_frames; i++){
    vm_map(kernel_pml4, frames+(i*PMM_BLOCK_SIZE),
        kmalloc_addr+(0x1000*i), 0);
  }

  kmalloc_addr += 0x1000*i;

  return ret_addr;
}

void vm_map(pagetable_t *pml4,
            physaddr_t physaddr, virtaddr_t vaddr, int flags)
{
  /* Get current paging structure */
  pagetable_t *pdp;
  pagetable_t *pdir;
  pagetable_t *pt;

  /* Get a lock & disable ints */
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(&vm_lock);

  /* Get appropriate pdp */
  pdp = vmm_getpdp(pml4, vaddr);
  if(pdp == 0)
  {
    /* Oh god, it isn't, allocate a new pdp */
    pdp = vmm_new_pagetable();

    vmm_cleartable(pdp);

    /* Install it */
    vmm_install_ptable(pml4, VMM_INDEX_PML4(vaddr),
        (physaddr_t)pdp, PAGE_PRESENT | PAGE_WRITE | flags);
  }

  /* Get appropriate pdir */
  pdir = vmm_getpdir(pdp, vaddr);
  if(pdir == 0)
  {
    /* Oh god, it isn't, allocate a new page directory */
    pdir = vmm_new_pagetable();
    vmm_cleartable(pdir);

    /* Install it */
    vmm_install_ptable(pdp, VMM_INDEX_PDP(vaddr),
        (physaddr_t)pdir, PAGE_PRESENT | PAGE_WRITE | flags);
  }

  /* Get appropriate page directory */
  pt = vmm_getptable(pdir, vaddr);
  if(pt == 0)
  {
    /* Oh god, it isn't, allocate a new page table */
    pt = vmm_new_pagetable();
    vmm_cleartable(pt);

    /* Install it */
    vmm_install_ptable(pdir, VMM_INDEX_PDIR(vaddr),
        (physaddr_t)pt, PAGE_PRESENT | PAGE_WRITE | flags);
  }

  /* NOW, FINALLY, Get the appropriate page */
  pt->pages[VMM_INDEX_PTABLE(vaddr)] = physaddr |
    PAGE_PRESENT | PAGE_WRITE | flags;

  /* Done, release lock */
  spinlock_release(&vm_lock);
  _interrupt_set_state(intr_status);

  /* Invalidate page in TLB cache */
  vmm_invalidatepage(vaddr);
  vmm_reloadcr3();
}

void vm_unmap(pagetable_t *pagetable, virtaddr_t vaddr)
{
  /* Unimplemented */
  vaddr = vaddr;
}

pagetable_t *vm_create_pagetable(uint32_t asid){
  asid = asid;

  //Get page table from pool
  pagetable_t *pml4 = vmm_new_pagetable();

  //copy the kernel mappings into the new page table
  memcopy(sizeof(pagetable_t), pml4, kernel_pml4);

  //every other level should be created under the first call to vm_map
  return pml4;
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
  /* Sanity check, is the pagetable pointer greater than the base
   * of the pagetable pool?
   *
   * Not sure if C allows comparison of pointers, so cast them.
   */
  if(((virtaddr_t)pagetable) < ((virtaddr_t)pt_pool))
    KERNEL_PANIC("vm_destroy_pagetable: Bad pointer range");

  //Calculate pagetable index
  uint64_t pt_index = pagetable - pt_pool;

  /* Sanity check, is the page table index less than the
   * number of page tables?
   */
  if(pt_index >= VM_PTP_SIZE)
    KERNEL_PANIC("vm_destroy_pagetable: Bad pointer ranger");

  //Unset bit in page pool bitmap
  ptmap_unsetbit(pt_index);
}

/* Compatability Functions */
uintptr_t _tlb_get_maxindex(void)
{
  return 512;
}
