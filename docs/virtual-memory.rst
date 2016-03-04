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
mechanism for allocating and
freeing physical pages. There is no swapping, the pagetables are inefficient to use
and hardware TLB is used in a very limited way. Kernel threads must also manipulate
allocated memory directly by pages. 
As result of this simple approach, the system can support only 16 pages of
mappings (64 kB) for each (userland) process. These 16 mappings can be fit into
the TLB and are currently put in place by calling ``tlb_fill`` after the
scheduler has changed threads. The system does not handle *TLB exceptions*,
and thus the kernel implementation does not use mapped memory.

Since kernel needs both virtual addresses for actual usage and physical address for
hardware, simple mapping macros are available for easy conversion. These macros
are ``ADDR_PHYS_TO_KERNEL()`` and ``ADDR_KERNEL_TO_PHYS()`` and they are defined in
vm/mips32/mem.h. Note that the macros can support only kernel region addresses
which are within the first 512MB of physical memory. See below for description on
address regions.

Hardware Support for Virtual Memory
-----------------------------------
The hardware in YAMS supports virtual memory with two main mechanisms: memory
segmentation and the TLB. The system doesn't support
hardware page tables. All page table operations and data structures are defined
by the operating system. The page size of the hardware is 4 KiB (4096 bytes). All
mappings are done in page sized chunks.
Memory segmentation means that addresses of different regions of the address space
behaves differently. The system has a 32-bit address space.
If the topmost bit of an address is 0 (the first 2GiB of address space), the address
is valid to use even if the CPU is in user mode (not in kernel mode). This region of
addresses is called the user mapped region and it is used by userland programs and
in the kernel when userland memory is manipulated. This region is mapped. Mapping
means that the addresses do not refer to real memory addresses, but the real memory
page is looked up from TLB when an address in this region is used. The TLB is
described in more detail in its own section (see section XX).
The rest of the address space is reserved for the operating system kernel and
will generate an exception if used while the CPU is in user (non-privileged) mode.
This space is divided into four segments: kernel unmapped uncached, kernel
unmapped, supervisor mapped and kernel mapped. Each segment is 512MiB in
size. The supervisor mapped region is not used in KUDOS. The kernel unmapped
uncached region is also not used in KUDOS except for memory mapped I/O-devices
(YAMS doesn’t have caches).
The kernel mapped region behaves just like the user mapped region, except that
it is usable only in kernel mode. This region can be used for mapping memory areas
for kernel threads. The area is currently unused, but its usage might be needed in
proper VM implementation.
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
(i.e.. ``stalloc()``) is disabled, so that ``stalloc()`` does not mess up the dynamically
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

``void physmem_freeblock(void \*ptr)``

* Frees a physical page by setting its corresponding bit to zero.
* Asserts that the page is reserved and that the page is not statically reserved.

Pagetables and Memory Mapping
-----------------------------
