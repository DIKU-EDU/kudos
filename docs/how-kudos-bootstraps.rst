How KUDOS Bootstraps
====================

This section explains the bootup process of KUDOS from the first instruction
ever executed by the CPU(s) to the point when userland processes can be
started.  This is **not** an introduction to *running* KUDOS in YAMS, for that,
we refer you to :doc:`using-kudos`.

Modern CPUs have at least two modes of operation: a *kernel mode*, which allows
for all assembly instructions, including *privileged instructions*, to be
executed, and *user mode*, which *does not* allow privileged instructions to be
executed.

Assembly code is both architecture-specific, and difficult to write and
maintain. We therefore wish to keep the amount of assembly code in the kernel
source code to the absolute minimum, running C code as soon as possible after
boot.

In order to run C code, an expandable region of memory called a "stack" is
required. When a function is called, depending upon the specific call
conventions, function parameters are *pushed* onto the stack (or placed in
registers), and space for local variables is allocated. The *return address* of
the function, i.e. the address of the next instruction to be executed after the
function completes execution is also pushed onto the stack.

The *stack pointer* is a CPU register that contains the memory address
corresponding to the top of the stack. The *program counter / instruction
pointer* registers contain the memory address of the next instruction to be
executed by the CPU. This instruction is fetched from memory, and the CPU
performs the corresponding operation on the values contained in the specified
CPU registers. Machine instructions can read and write data from specified
memory addresses into CPU registers.

.. Booting ``kudos-mips32`` in YAMS
.. --------------------------------
.. 
.. When YAMS is powered up, the program counter register for every CPU (YAMS can
.. simulate multiple CPU cores) is set to ``0x80010000``. This is where the
.. ``.text`` segment of ``kudos-mips32`` begins, i.e. where the first
.. ``kudos-mips32`` instruction is stored.
.. 
.. All MIPS32-specific bootstrapping code is found in ``kudos/init/mips32/``.
.. 
.. The assembly code in ``_boot.S`` is the very first code that ``kudos-mips32``
.. will execute. The processor number is detected and all processors except number
.. 0 will enter a wait loop until kernel initialization is finished. Later, when
.. the kernel initialization (in ``main.c``) is complete, processor 0 will signal
.. the other processors to continue.
.. 
.. The first thing that the ``boot.S`` code will do is processor separation. The
.. processor number is detected and all processors except number 0 will enter a
.. wait loop waiting for the kernel initialization to be finished. Later, when the
.. kernel initialization (in ``main.c``) is finished, processor 0 will signal the
.. other processors to continue.
.. 
.. The stack pointer is set to ``0x8000fffc``, which is just below the kernel
.. image. This provides a temporary stack for the init C code. Later, each kernel
.. thread will have its own stack area.
.. 
.. Once the init stack has been set up, we can jump to the ``init`` function in
.. the (still) architecture-specific ``main.c``.

Booting KUDOS/x86_64 with GRUB2
-------------------------------

On boot, the BIOS, which is mapped to a specific location in memory, is run.
The BIOS detects what hardware is present, placing this information in memory,
and runs a bootloader on a specified device, e.g. a hard disk.

GRUB is a generic bootloader, which can be used by operating systems that
support the `multiboot specification
<https://www.gnu.org/software/grub/manual/multiboot/multiboot.html>`_, such as
KUDOS. When the bootloader starts, the CPU is in 16-bit real mode.

GRUB loads a kernel image, and begins its execution at the *entry point*, which
was defined when linking, by setting the instruction pointer to this memory
address. It starts this execution in 32-bit *protected mode*. Protected mode
provides memory protection, i.e. user and kernel modes, such that processes
cannot interfere with one another's execution. This allows for a multi-tasking
operating system, as the kernel is protected from interference by processes
running in user mode.

There is a clash of terminology here; a CPU in protected mode has the ability
to switch between user and kernel mode, whereas real mode provides no such
protection.

The entry point is ``kudos/init/x86_64/_boot.S``, which sets up long mode (64
bit mode), and a stack for the execution of C code, before jumping to the
architecture-specific ``init`` function, located in
``kudos/init/x86_64/main.c``.

Starting Subsystems
-------------------

To provide operating system service, various subsystems are required, in order
to coordinate resource usage. These are started in the respective,
architecture-specific ``init`` functions in ``init/$ARCH/main.c``.

``stalloc``
  Provides permanent, static kernel memory allocation.

``polltty``
  Allows input from the keyboard, and text output to the display. This is a
  *polling* device, so getting input/putting output requires repeatedly checking
  the status of the keyboard/display in a busy-wait loop.

``interrupt``
  Allows devices and user processes to *interrupt* the CPU when they have an
  event that must be handled. This can be used to prevent the CPU from having to
  poll. The same subsystem handles *exceptions*.

``thread_table``
  Allows the operating system to have mutiple threads of execution. It is
  initialised, by creating a *thread table* to keep track of the threads. A
  *thread* of execution is (the state of) some running code, e.g. registers such
  as the instruction pointer.

``sleep_queue``
  A syncronization mechanism, allowing threads to wait on resources currently
  in use by other threads.

``semaphore``
  Another, high-level, inter-thread syncronization mechanism.

``device``
  Stores information about I/O devices, such as the TTY device, in a *device
  table*. In KUDOS, devices implement generic interfaces, minimizing code
  duplication.

``vfs``
  The Virtual File-System (VFS) is a generic file-system abstraction over more
  specific file-systems, such as the KUDOS Trivial File System (TFS).

``sheduler``
  Performs the actual switching of threads on and off CPU cores, saving the
  context of the thread, such as its registers.

``vm``
  Allows the operating system to place pages non-contiguously in memory, by
  mapping addresses from *physical addresses* to *virtual addresses*. Once this
  subsystem has been initialised, the ``stalloc`` subsystem is disabled.

Finally, a new thread is created and run on the CPU instead of the current
thread. On ``kudos-mips32`` the other CPUs are now released from the wait-loop.
The new thread executes the architecture-independent function
``init_startup_thread`` (defined in ``kudos/init/common.c``) which sets up the
last two things:

1. All filesystems implementing the VFS interface are made available, or
   mounted, by ``vfs_mount_all``.

2. The program corresponding to the ``initprog`` argument given at boot is
   loaded into memory, and execution continues at the address of the first
   instruction in this program.  If no ``initprog`` argument was given, the
   function ``init_startup_fallback`` is called instead.
