Virtual Memory
==============
By definition, Virtual Memory (VM) provides an illusion of unlimited sequential memory
regions to threads and processes. Also the VM subsystem should isolate processes
so that they cannot see or manipulate memory allocated by other processes. The
current KUDOS implementation does not achieve these goals. Instead, it provides
tools and utility functions which are useful when implementing a real and working
virtual memory subsystem.

Currently the VM subsystem has primitive page tables for threads and processes,
utilities to manipulate hardware Translation Lookaside Buffer (TLB) and a simple
mechanism for allocating and freeing physical pages. There is no swapping, the
pagetables are inefficient to use and hardware TLB is used in a very limited
way. Kernel threads must also manipulate allocated memory directly by pages.

As result of this simple approach, the system can support only 16 pages of
mappings (64 kB) for each (userland) process. These 16 mappings can be fit into
the TLB and are currently put in place by calling ``tlb_fill`` after the
scheduler has changed threads. The system does not handle *TLB exceptions*,
and thus the kernel implementation does not use mapped memory.

A proper system uses virtual addresses for processes, and uses physical
addresses for hardware.  Because of this, simple mapping macros are available
for easy conversion. These macros are ``ADDR_PHYS_TO_KERNEL()`` and
``ADDR_KERNEL_TO_PHYS()``, defined in `kudos/vm/mips32/mem.h`. Note that the
macros can support only kernel region addresses which are within the first 512MB
of physical memory. See below for description on address regions.

Hardware Support for Virtual Memory
-----------------------------------

The hardware in YAMS supports virtual memory with two main mechanisms: memory
segmentation and the TLB. The system doesn't support
hardware page tables. All page table operations and data structures are defined
by the operating system. The page size of the hardware is 4 KiB (4096 bytes). All
mappings are done in page-sized chunks.

Memory segmentation means that addresses of different regions of the address space
behaves differently. The system has a 32-bit address space.

If the topmost bit of an address is 0 (the first 2GiB of address space), the address
is valid to use even if the CPU is in user mode (i.e. not in kernel mode). This region of
addresses is called the "user-mapped region" and it is used by userland programs and
in the kernel when userland memory is manipulated. This region is mapped. Mapping
means that the addresses do not refer to real memory addresses, but the real memory
page is looked up from TLB when an address in this region is used. The TLB is
described in more detail in its own subsection.

The rest of the address space is reserved for the operating system kernel and
will generate an exception if used while the CPU is in user (non-privileged) mode.
This space is divided into four segments: kernel unmapped uncached, kernel
unmapped, supervisor mapped and kernel mapped. Each segment is 512MiB in
size. The supervisor mapped region is not used in KUDOS. The kernel unmapped
uncached region is also not used in KUDOS except for memory mapped I/O-devices
(YAMS doesn’t have caches).

The kernel-mapped region behaves just like the user-mapped region, except that
it is usable only in kernel mode. This region can be used for mapping memory areas
for kernel threads. The area is currently unused, but its usage might be needed in
a proper VM implementation.

The kernel unmapped region is used for static data structures in the kernel and
also for the kernel binary itself. The region maps directly to the first 512MiB of
system memory (just strip the most significant bit of an address).
In some parts of the system the term *physical memory address* is used. Physical
addresses starts from 0 and extends to the top of the machine’s
real memory. These are used for example in the TLB to point to actual pages of memory
and in device drivers when doing DMA data transfers.

.. figure:: yams-virtual-memory.png

   YAMS virtual memory layout

KUDOS Virtual Memory Initialization
-----------------------------------

During the virtual memory Initialization (functions ``vm_init`` and ``physmem_init``) a
bitmap data structure ``physmem_free_pages`` is created to keep track of available
physical memory pages. The ability to do arbitrary length permanent memory reservations
(i.e. ``stalloc()``) is disabled, so that ``stalloc()`` does not mess up the dynamically
reserved pages.

Page bitmap
<<<<<<<<<<<

The page bitmap (also called a *page pool*) is a data structure containing the status
of all physical pages.
The status of a physical page is either free or reserved. The status information
of the *n*'th page is kept in the *n*'th bit in the page bitmap, zero meaning *free* and
one meaning *reserved*.

A spinlock is provided to secure the synchronous access to the page bitmap. It is 
need to prevent two or more threads from reserving the same physical page.

``void vm_init ()``

* Initialize the virtual memory and disable ``stalloc()``
* Implementation:
    1. Call ``physmem_init()``
    2. Disable future calls to ``stalloc()``

``void physmem_init ()``

* Initialize the page bitmap. After this, it is known which pages may be used by virtual memory system for dynamic memory reservation. Statically reserved pages are marked as reserved.
* Implementation: 
    1. Find the total number of physical pages.
    2. Reserve space for the page bitmap ``physmem_free_pages``, note that this is still a permanent memory reservation.
    3. Get the number of statically reserved memory pages, reserved by ``stalloc()``.
    4. Mark all statically reserved pages in the page bitmap as ones.

The following page handling functions is provided to manipulate the page bitmap:

``physaddr_t physmem_allocblock(void)``

* Returns the physical address of the first free page and marks it as
  reserved. Returns zero, if no free page is available.
* The function finds the first zero bit from the page bitmap, and marks it to
  one. The physical address is calculated by multiplying the bit number with the
  page size.

``void physmem_freeblock(void *ptr)``

* Frees a physical page by setting its corresponding bit to zero.
* Asserts that the page is reserved and that the page is not statically reserved.

Pagetables and Memory Mapping
-----------------------------

+---------------------------------------+-----------------------+
| Type                                  | Name                  |
+=======================================+=======================+
| | ``uint32_t``                        | | ``ASID``            |
| | ``uint32_t``                        | | ``valid_count``     |
| | ``tlb_entry_t[PAGE_TABLE_ENTRIES]`` | | ``entries``         |
+---------------------------------------+-----------------------+

Table of ``pagetable_t``.

* ``ASID``: Address space identifier. The entries placed in the TLB will be set with this ASID. Only entries in the TLB with an ``ASID`` matching with the ``ASID`` of the currently running thread will be valid. In KUDOS we use ``ASID == Thread ID``.
* ``valid_count``: Number of valid mapping entries in this pagetable.
* ``tlb_entry_t[PAGE_TABLE_ENTRIES]``: The actual page mapping entries in the form accepted by the hardware TLB.

KUDOS uses very primitive pagestables to store memory mappings for userlands processes.
Each thread entry in kudos has a private pagetable entry (``pagetable_t *pagetable``)
in its data structur (``thread_table_t``).
If the entry is ``NULL``, then the thread is a kernel-only thread. If the entry is not ``NULL``,
then the thread is used in userland.

The pagetable stores virtual address physical address mapping pairs for the
process. Virtual addresses are private for the process, but physical addresses are
global and refer to actual physical memory locations. The pagetable is stored in
``pagetable t`` structure described in the table above.

Before a thread can use memory mapping, the thread must create a pagetable by calling the function
``vm_create_pagetable()`` giving its thread ID as the argument. This pagetable
is then stored in thread’s information structure. For an example on usage, see
``process_start()`` in ``proc/process.c``. Note that the current VM implementation
cannot handle TLB dynamically, which means that TLB must be filled
with proper mappings manually before running threads (userland processes) which
needs them. This can be achieved by calling ``tlb_fill()`` (see ``proc/mips32/_proc.c``:
``process_set_pagetable()`` and ``kernel/mips32/interrupt.c``: ``interrupt_handle()`` for current
usage).

When the thread no longer needs its memory mappings, it must destroy its
pagetable by calling ``vm_destroy_pagetable()``. Note that this only clears the mappings,
but does not invalidate the pagetable entry in thread information structure,
free the physical pages used in mappings or clear the TLB. These things must be
handled by the thread wishing to free memory (eg. a dying userland process).

``pagetable t * vm create pagetable (uint32 t asid)``

* Create a new pagetable. Returns a pointer to the newly created pagetable.
* Argument ``asid`` defines the address space identifier associated with this page table. In KUDOS we use asids which equal to thread IDs.
* A ``pagetable_t`` occupies one hardware page (4096 bytes).
* Implementation:
    1. Reserve one physical memory page, this page will contain one ``pagetable_t`` structur.
    2. Set the ``ASID`` field in the newly created ``pagetable_t`` structur.
    3. Set the number of valid mappings to zero.
    4. Return a pointer to the newly created ``pagetable_t`` structur.

``void vm_destroy_pagetable(pagetable_t *pagetable)``

* Frees the given ``pagetable_t`` structur.
* The pagetable must not be used after it has been freed. The freeing is done when a userland
  process terminates.
* Note: that this function does not invalidate any entries present in the TLB.
* Implementation:
    1. Free the page used for the ``pagetable_t`` structur, by calling the ``physmem_freeblock()`` function.

Memory mappings can be added to pagetables by calling the ``vm_map()``. The current TLB
implementation cannot handle more than 16 pagetable mappings currectly. Mappings can be removed
one by one with the ``vm_unmap()`` function. The dirty bit of a mapping can be changed by calling
``vm_set_dirty()``.

``vm_map(pagetable_t *pagetable, physaddr_t physaddr, virtaddr_t vaddr, int flags)``

* Maps the given virtual address (``vaddr``) to the given physical address (``physaddr``) in the context of the given pagetable. The addresses must be page aligned (4096 bytes).
* If ``dirty`` is ``true``, the mapping is marked dirty (read/write mapping). If ``false``, the mapping will be clean (read-only).
* Implementation:
    1. If the pagetable already contains the pair entry for the given virtual address (page), the pair entry is filled. Pagetables use the hardware TLB’s mapping definitions where even and odd pages are mapped to the same entry but can point to different physical pages.
    2. Else creates new mapping entry, fills the appropriate fields and invalidates the pairing (not yet mapped) entry.

``void vm_unmap(pagetable_t *pagetable, virtaddr_t vaddr)``

* Unmaps the given virtual address (``vaddr``) from given pagetable. The address must be page aligned and mapped in this pagetable.
* Implementation:
    1. This function is not implemented.

``void vm_set_dirty(pagetable_t *pagetable, virtaddr_t vaddr, int dirty)``

* Sets the dirty bit to ``dirty`` of a given virtual address (``vaddr``) in the context of the given pagetable. The address must be page aligned (4096 bytes).
* If ``dirty`` is ``true`` (1), the mapping is marked dirty (read/write mapping). If ``false`` (0), the mapping will be clean (read-only).
* Implementation:
    1. Find the mapping of the given virtual address.
    2. Set the dirty bit, if the mapping is found.
    3. If the mapping is not found, ``PANIC``.

TLB
---

Most modern processors access virtual memory through a Translation Lookaside
Buffer (TLB). It is an associative table inside the memory management unit (MMU,
``CP0`` in MIPS32) which consists of a small number of entries similar to page table
entries mapping virtual memory pages to physical pages.

When the address of a memory reference falls into a mapped memory range
(``0x00000000-0x7fffffff`` or ``0xc0000000-0xffffffff`` in MIPS) the virtual page
of the address is translated into a physical page by the MMU hardware by looking
it up in the TLB and the resulting physical address is used for the reference. If the
virtual page has no entry in the TLB, a TLB exception occurs.

TLB dual entries and ASID in MIPS32 architectures
-------------------------------------------------

In the MIPS32 architecture, one TLB entry always maps two consecutive pages, even
and odd. This needs to be taken into account when implementing the TLB handling
routines, as a new mapping may need to be added to an already existing TLB entry.
One might think that the consecutive pages could be mapped in separate entries,
leaving the other page in the entry as invalid, but this would result in duplicate
TLB matches and thus cause undefined behavior.

A MIPS32 TLB entry also has an Address Space ID (``ASID``) field. When the
``CP0`` is checking for a TLB match, the ``ASID`` of the entry must match the
current ``ASID`` for the processor, specified in the ``EntryHi`` register (or the global bit
is on). Thus, when using different
``ASID`` for each thread, the TLB need not necessarily be invalidated when switching
between threads.

KUDOS uses the ``tlb_entry_t`` structure to store page mappings. The entries in
this structure are compatible with the hardware TLB. The fields are described in
table below.

The exception handler in ``kernel/mips32/exception.c`` should dispatch TLB exceptions
to the following functions, implemented in ``vm/mips32/tlb.c`` (note that the current implementation
does not dispatch TLB exceptions):

``void tlb_modified_exception(void)``

* Called in case of a TLB modified exception.

``void tlb_load_exception(void)``

* Called in case of a TLB miss exception caused by a load reference.

``void tlb_store_exception(void)``

* Called in case of a TLB miss exception caused by a store reference.

TLB miss exception, Load reference
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

The cause of this exception is a memory load operation for which either no entry
was found in the TLB (TLB refill) or the entry found was invalid (TLB invalid).
These cases can be distinguished by probing the TLB for the failing page number.
The exception code is ``EXCEPTION_TLBL``.

TLB miss exception, Store reference
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

This exception is the same as the previous except that the operation which caused
it was a memory store. The exception code is ``EXCEPTION_TLBS``.

TLB modified exception
<<<<<<<<<<<<<<<<<<<<<<

This exception occurs if an entry was found for a memory store reference but the
entry’s D bit is zero, indicating the page is not writable. The D bit can be used both
for write protection and pagetable coherence when swapping is enabled (dirty/not
dirty). The exception code is ``EXCEPTION_TLBM``.

TLB wrapper functions in KUDOS
<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

The following wrapper functions to CP0 TLB operations, implemented in ``vm/mips32/_tlb.S``,
are provided so that writing assembler code is not required.

``void _tlb_get_exception_state(tlb_exception_state_t *state)``

* Get the state parameters for a TLB exception and place them in state.
* This is usually the first function called by all TLB exception handlers.
* Implementation:
    1. Copy the ``BadVaddr`` register to ``state->badvaddr``.
    2. Copy the ``VPN2`` field of the *EntryHi* register to ``state->badvpn2``.
    3. Copy the ``ASID`` field of the *EntryHi* register to ``state->asid``.

``void _tlb_set_asid(uint32_t asid)``

* Sets the current ASID for the CP0 (in EntryHi register).
* Used to set the current address space ID after operations that modified the EntryHi register.
* Implementation:
    1. Copy ``asid`` to the *EntryHi* register.

``uint32_t _tlb_get_maxindex(void)``

* Returns the index of the last entry in the TLB. This is one less than the number of entries in the TLB.
* Implementation:
    1. Return the MMU *size* field of the *Conf1* register

``int _tlb_probe(tlb_entry_t *entry)``

* Probes the TLB for an entry defined by the ``VPN2``, ``dummy1`` and ``ASID`` fields of entry.
* Returns an index to the TLB, or a negative value if a matching entry was not found.
* Implementation:
    1. Load the *EntryHi* register with ``VPN2`` and ``ASID``.
    2. Execute the TLBP instruction.
    3. Return the value in the Index register.

``int _tlb_read(tlb_entry_t *entries, uint32_t index, uint32_t num)``

* Reads ``num`` entries from the TLB, starting from the entry indexed by index. The entries are placed in the table addressed by entries.
* Only ``MIN(TLBSIZE-index, num)`` entries will be read.
* Returns the number of entries actually read, or a negative value on error.
* Implementation:
    1. Load the Index register with index.
    2. Execute the TLBR instruction.
    3. Move the contents of the *EntryHi*, *EntryLo0* and *EntryLo1* registers to corresponding fields in entries.
    4. Advance index and entries, and continue from step 1 until enough entries are read.
    5. Return the number of entries read.

``int _tlb_write(tlb_entry_t *entries, uint32_t index, uint32_t num)``

* Writes ``num`` entries to the TLB, starting from the entry indexed by index. The entries are read from the table addressed by entries.
* Only ``MIN(TLBSIZE-index, num)`` entries will be written.
* Returns the number of entries actually written, or a negative value on error.
* Implementation:
    1. Load the *Index* register with index.
    2. Fill the *EntryHi*, *EntryLo0* and *EntryLo1* registers from entries.
    3. Execute the TLBWI instruction.
    4. Advance index and entries, and continue from step 1 until enough entries are written.
    5. Return the number of entries written.

``void _tlb_write_random(tlb_entry_t *entry)``

* Writes the entry to a "random" entry in the TLB. The entry is read from entry.
* Note that if this function is called more than once, it is not guaranteed that the newest write will not overwrite the previous, although this is usually the case. This function should only be called to write a single entry.
* Implementation:
    1. Fill the *EntryHi*, *EntryLo0* and *EntryLo1* registers from entry.
    2. Execute the TLBWR instruction. The following function should be used only until a proper VM implementation is done:

+--------------------+----------------+
| Type               | Name           |
+====================+================+
| | ``unsigned int`` | | ``VPN2:19``  |
| | ``unsigned int`` | | ``dummy1:5`` |
| | ``unsigned int`` | | ``ASID:8``   |
| | ``unsigned int`` | | ``dummy2:6`` |
| | ``unsigned int`` | | ``PFN0:20``  |
| | ``unsigned int`` | | ``C0:3``     |
| | ``unsigned int`` | | ``D0:1``     |
| | ``unsigned int`` | | ``V0:1``     |
| | ``unsigned int`` | | ``G0:1``     |
| | ``unsigned int`` | | ``dummy3:6`` |
| | ``unsigned int`` | | ``PFN1:20``  |
| | ``unsigned int`` | | ``C1:3``     |
| | ``unsigned int`` | | ``D1:1``     |
| | ``unsigned int`` | | ``V1:1``     |
| | ``unsigned int`` | | ``G1:1``     |
+--------------------+----------------+

Table of ``tlb_entry_t``.

* ``VPN2``: Virtual page pair number. These are the upper 19 bits of a virtual address. VPN2 describes which consecutive 2 page (8192 bytes) region of virtual address space this entry maps.
* ``dummy1``: Unused.
* ``ASID``: Address space identifier. When ``ASID`` matches CP0 setted ``ASID`` this entry is valid. In KUDOS, we use mapping ``ASID = thread_id``.
* ``dummy2``: Unused.
* ``PFN0``: Physical page number for even page mapping (VPN2 + 0 bit).
* ``C0``: Cache settings. Not used.
* ``D0``: Dirty bit for even page. If this is 0, page is write protected. If 1 the page can be written to.
* ``V0``: Valid bit for even page. If this bit is 1, this entry is valid.
* ``G0``: Global bit for even page. Cannot be used without the global bit of odd page.
* ``dummy3``: Unused.
* ``PFN1``: Physical page number for odd page mapping (VPN2 + 1 bit).
* ``C1``: Cache settings. Not used.
* ``D1``: Dirty bit for odd page. If this is 0, page is write protected. If 1 the page can be written to.
* ``V1``: Valid bit for odd page. If this bit is 1, this entry is valid.
* ``G1``: Global bit for odd page. Cannot be used without the global bit of even page. If both bits are 1, the mapping is global (ignores ASID), otherwise mapping is local (checks ASID).

+----------------+----------------+
| Type           | Name           |
+================+================+
| | ``uint32_t`` | | ``badvaddr`` |
| | ``uint32_t`` | | ``badvpn2``  |
| | ``uint32_t`` | | ``asid``     |
+----------------+----------------+

Table of ``tlb_exception_state_t``.

* ``badvaddr``: Contains the failing virtual address.
* ``badvpn2``: Contains the VPN2 (bits 31..13) of the failing virtual address
* ``asid``: Contains the ASID of the reference that caused the failure. Only the lowest 8 bits are used.
