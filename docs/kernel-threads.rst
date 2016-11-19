Kernel Threads
==============

A *thread of execution* is the execution of a sequence of instructions. The
*context* of a thread is the contents of the CPU registers at a given point of
execution, including things like the *program counter*, *stack pointer*, and
*co-processor* registers. A thread may be *interrupted*, or *pre-empted*, its
context stored in memory, only to be restored, and thread *re-entered* at a
later point in time. A thread may be pre-empted to let another thread re-enter
and do some other useful work. A *scheduler* decides which thread gets to go
next.

On a *uniprocessor* system threads offer the illusion of having multiple,
co-operating CPUs, while offering truly *concurrent* execution on a
multiprocessor system.

Code for a *multithreaded* system must be written in a *re-entrant* fashion,
i.e. such that execution may be interrupted and re-entered at any given point,
except within otherwise demarcated *critical regions* of the code. Code for a
*multiprocessor* system must furthermore ensure *exclusive access* to data
structures shared across multiple threads of execution.

KUDOS is designed to be a multithreaded system. It achieves this goal in two
fundamental ways: It has a *pre-emptive*, *round-robin* scheduler, and is
designed to be a *symmetric multiprocessing (SMP)* system, which means that it
supports multiple CPU cores, running each either own thread, while sharing the
same physical memory.

Threads are a fundamental kernel construct, which allows to implement more
nuanced userland threads of execution, such as userland threads and userland
processes.

Kernel Threads API
------------------

The kernel threads API, defined in ``kudos/kernel/thread.h`` and implemented in
``kudos/kernel/thread.c``, provides functions for setting up kernel threads in
the kernel thread table, and for kernel threads to interact with the scheduler.

``void thread_table_init(void)``
  Initialisation of thread table and idle thread for threading subsystem.

The following two functions are used by a thread to create a new thread, and
mark it as ready to run:

``TID_t thread_create(void (*func)(uint32_t), uint32_t arg)``
  Finds the first free entry in the thread table, and sets up a thread for the
  given function.  This function does not cause the thread to be run, and the
  thread’s resultant state is ``NONREADY``.

``void thread_run(TID_t t)``
  Causes the thread’s state in the thread table to be updated to READY,
  allowing the scheduler to allocate the thread to a CPU core.

The following function can be used by a kernel thread to manipulate itself:

``void thread_switch(void)``
  Perform voluntary context switch. An interrupt is invoked, causing the scheduler to
  reschedule. Interrupts must be enabled when this function is called, and the interrupt
  state is restored prior to the function returning.

``void thread_yield(void)``
  Macro pointing to ``thread_switch``. The name "switch" is used when, for
  instance, the thread goes to sleep, whereas the name yielding implies no
  actual effect.

``void thread_finish(void)``
  Called automatically when the thread's function terminates or voluntarily by
  the thread to "commit suicide". Tidies up after thread.

``TID t thread_get_current_thread(void)``
  Returns the TID of the calling thread.

Controlling Kernel Threads
--------------------------

To keep track of threads, a thread table is used. This is a fixed size array of
elements of type ``thread_table_t``, each entry being a structure describing
one thread. The size of this array, or maximum possible number of threads, is
defined by ``CONFIG_MAX_THREADS`` in ``kudos/kernel/config.h``. The thread
table itself is ``thread_table``, defined in ``kudos/kernel/thread.c``. The
index into ``thread_table_t`` corresponds to the kernel *thread id* (TID).

A ``thread_table_t`` has (among others) the following fields:

``context_t *context``
  Thread context, i.e. all CPU registers, including the program counter (PC)
  and the stack pointer (SP), which always refers to the thread's stack area.

``context_t *user_context``
  Pointer to thread's userland context; ``NULL`` for kernel-only threads.

``thread_state_t state``
  The current state of the thread. Possible values are: ``FREE``, ``RUNNING``,
  ``READY``, ``SLEEPING``, ``NONREADY`` and ``DYING``.

.. ``uint32_t sleeps_on``
..   If non-zero, specifies which resource the thread is sleeping on (waiting
..   for), i.e.  the thread is in some list in the sleep queue. The thread may
..   still be ``RUNNING``, and in the process of going to sleep.

``pagetable_t *pagetable``
  Pointer to the virtual memory mapping for this thread; ``NULL`` if the thread
  does not have a page table.

``pid_t pid``
  Process identifier for corresponding userland process. Thread creation sets
  this to a negative value.

``_kthread_t thread_data``
  Padding to 64 bytes for context switch code. If structure is modified, the
  architecture-specific paddings in ``_thread.h`` must be updated.

A thread may correspond to a userland process, and the thread table then stores
the process' context, page table, and PID. For cross-architecture
compatibility, architecture-specific padding is defined in the
architecture-specific ``_thread.h``.

As the thread table is shared between all threads, of which several may be
executing concurrently, it must be protected from concurrent updates, using a
kernel lock.
