Kernel Overview
===============

Although aiming for simplicity, the KUDOS kernel is still quite a complicated
piece of software.

To understand how the kernel is built, we must first understand what it actually
does.  The kernel is a mediator between userland processes and machine hardware
to provide services for processes.  It is also responsible for providing the
userland processes a private sandbox in which to run.  Further, the kernel also
provides various high level services such as filesystems, which act on top of
the raw device drivers.

On the kernel side of these boundaries lies the important system call interface
code.  The system call interface is a set of functions which can be called from
userland programs.  These functions can then call almost any function inside the
kernel to implement the required functionality.  Kernel functions cannot be
called directly from userland programs, which protects kernel integrity and
makes sure that the userland sandbox doesn't leak.

The kernel also contains the device drivers.  Device drivers are pieces of code
which know how to use a particular piece of hardware.  Device drivers are
usually divided into two parts: the top and bottom halves.  One part reacts on
hardware signals (an "interrupt handler"), and the other part is a set of
functions which can be called from within the kernel to send signals to the
hardware.


Threading
---------

Now we have seen an overview of various kernel services, but we still don't have
anything which can call these service functions.  The core of any kernel,
including KUDOS, is its threading and context switching functionality.
Threading is provided by a special threading library in KUDOS.  The threading
system makes it possible to execute threads in separate instances of program
execution.  Each thread runs independently of each other, alternating their
turns on the CPU(s).  The context switching system is used to switch one thread
out of a CPU and to put a new one on it.  Threads themselves are unaware of
these switches, unless they intentionally force themselves out of execution,
i.e. go to sleep.

When starting a thread it is given a function which it executes.  When the
function ends, the thread dies.  The thread can also commit suicide by
explicitly killing itself. The kernel does not allow threads to kill other
threads. Each userland program runs inside one thread.  When the actual
userland code is being run, the thread cannot see the kernel memory.  It can
only use the system call layer.

Threads can be pre-empted at any point, both when in kernel and when in user
mode.  Pre-empting means that the thread is taken out of execution in favor of
some other thread.  The only way to prevent pre-empting is to disable
interrupts.

Since the kernel includes many data structures, and since multiple threads can
be run simultaneously (we can have multiple CPUs), all data has to be protected
from other threads.  The protection can be done with exclusive access, achieved
with various synchronization mechanisms.


Virtual Memory
--------------

The virtual memory subsystem in KUDOS affects the whole kernel, interacts with
hardware and also with the userland.

The VM subsystem is responsible for all memory handling operations in the
kernel.  Its main function is to provide an illusion of private memory spaces
for userland processes, but its services are also used in the kernel.  Since
memory can be accessed from any part of the system, virtual memory interacts
directly with all system components.

.. figure:: kudos-memory.png

   The KUDOS memory structure.

The physical memory usage in KUDOS for its ``mips32`` target can be seen in the
picture above.  At the left side of the figure, memory addresses can be seen.
At the bottom is the beginning of the system main memory (address zero) and at
the top the end of the physical memory.

The kernel uses part of this physical memory for its code (kernel image),
interrupt handling routines and data structures, including thread stacks. The
rest of the memory is at the mercy of the VM.

As in any modern hardware, memory pages (4096 byte regions in our case) can be
*mapped* in YAMS.  The mapped addresses are also called *virtual addresses*.
Mapping means that certain memory addresses do not actually refer to physical
memory.  Instead, they are references to a structure which *maps* these
addresses to the actual addresses.  This makes it possible to provide the
illusion of exclusive access to userland processes.  Every userland process has
code at address ``0x00001008``, for example.  In reality this address is in the
mapped address range, and thus the code is actually on a private physical memory
page for each process.


Support for Multiple Processors
-------------------------------

KUDOS is a multiprocessor operating system, with pre-emptive
kernel threading. All kernel functions are thread-safe (re-entrant)
except for those that are used only during the bootup process.

Most code explicitly concerning SMP support is found in the bootstrap code.
Unlike in real systems, where usually only one processor starts at boot and it
is up to it to start the other processors, in YAMS *all* processors will start
executing code simultaneously and at the same address (``0x80010000``).  To
handle this, each processor must "know" its own number, to facilitate code that
branches to different locations based which CPU (otherwise all CPUs would be
doomed to run the same code forever, which is plainly inefficient).

Another place where the SMP support is directly evident is in the context switch
code, and in the code initializing data structures used by the context switching
code.  Each processor must have its own stack when handling interrupts, and each
processor has its own current thread.  To account for these, the context
switching code must know the processor on which it runs.

For the most part, the SMP support should be completely transparent, although it
means that synchronization issues must be handled more carefully.


Kernel programming
------------------

Kernel programming differs somewhat from programming user programs.

The most significant difference is memory usage.  In the MIPS32 architecture,
which YAMS emulates, the memory is divided into segments.  Kernel code can
access all these segments, while user programs can only access the first
segment called the *user mapped segment*.  In this segment the first bit of the
address is ``0``.  If the first bit is ``1``, the address belongs to one of the
kernel segments and is not usable in userland.  The most important kernel
segment in KUDOS is the *kernel unmapped segment*, where addresses start with
the bit sequence ``100``.  These addresses point to physical memory locations.
In the kernel, most addresses are like this.

For initializing the system, KUDOS provides a function ``kmalloc`` (for "kernel
malloc") to allocate memory in arbitrary-size chunks.  This memory is
permanently allocated and cannot be freed.  Before initializing the virtual
memory system, ``kmalloc`` is used to allocate memory.  After the initialization
of the virtual memory system, ``kmalloc`` can no longer be used.  Instead,
memory is allocated page by page from the virtual memory system.  These pages
can be freed later.


Stacks and contexts
-------------------

A stack is always needed when running code that is written in C (otherwise we
cannot have C functions).  The kernel provides a valid stack for user programs
so the programmer does not need to think about this.  In the kernel, however, no
one provides us with a valid stack.  Every kernel thread must have its own
stack.  In addition, every CPU must have an interrupt stack because thread
stacks cannot be easily used for interrupt processing.  If a kernel thread is
associated with a user process, the user process must also have its own stack.
KUDOS already sets up kernel stacks and interrupt stacks appropriately.

Because the kernel and interrupt stacks are statically allocated, their size is
limited. This means that large structures and tables cannot be allocated on the
stack (in C, the variables declared inside a function are stack-allocated).
Note also that recursive functions allocate space from the stack for each
recursion level. Deeply recursive functions should thus not be used.

Code can be run in several different contexts.  A context consists of a stack
and CPU register values.  In the kernel there are two different contexts.
Kernel threads are run in a normal kernel context with the thread's stack.
Interrupt handling code is run in an interrupt context with the CPU's interrupt
stack.  These two contexts differ in a fundamental way.  In the kernel context
the current context can be saved and resumed later.  Thus interrupts can be
enabled, and blocking operations can be called.  In the interrupt context this
is not possible, so interrupts must be disabled, and no blocking operations can
be called.  In addition, if a kernel thread is associated with a userland
process, it must also have a userland context.


Working with a text input/output console
----------------------------------------

In the kernel, reading from and writing to the console is done by using the
polling TTY driver.  The ``kprintf`` and ``kwrite`` functions can be used to
print informational messages to the user.  Userland console access should not be
handled with these functions.  The interrupt driven TTY driver should be used
instead.


Busy waiting
------------

In the kernel, special attention has to be given to synchronization issues.
Busy waiting must be avoided whenever possible.  The only place where busy
waiting is acceptable is when using the spinlock implementation, which is
already implemented for you.  Because spinlocks use busy waiting, they should
never be held for a long time.


Floating point numbers
----------------------

YAMS does not support floating point numbers, so they cannot be used with
KUDOS' ``mips32`` target either.  If an attempt to execute a floating point
instruction is made, a co-processor unusable exception will occur (since the
floating point unit is co-processor 1 in the MIPS32 architecture.)


Naming conventions
------------------

Some special naming conventions have been used when programming KUDOS.  These
might help you find a function or a variable when you need it.  Functions are
generally named as ``filename_action`` -- where ``filename`` is the name of the
file where the function resides, and ``action`` tells what the function does.
Global variables are named similarly.


C calling conventions
---------------------

Normally, a C compiler handles function calling conventions (mostly argument
passing) transparently.  Sometimes in kernel code the calling convention issues
need to be handled manually.  Manual calling convention handling is needed when
calling C routines from an assembly program or when manipulating thread
contexts in order to pass arguments to starting functions.

Arguments are passed to all functions in MIPS32 argument registers ``A0``,
``A1``, ``A2`` and ``A3``.  When more than 4 arguments are needed, the rest are
passed on the stack.  The arguments are put onto the stack so that the 1st
argument is in the lowest memory address.

There is one thing to note: the stack frame for arguments must always be
reserved, even when all arguments are passed in the argument registers. The
frame must have space for all arguments.  Arguments which are passed in
registers need not to be copied into this reserved space.
