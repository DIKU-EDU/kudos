Threads
=======

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
except within otherwise demarkated *critical regions* of the code. Code for a
*multiprocessor* system must futhermore ensure *exclusive access* to data
structures shared across multiple threads of execution.

KUDOS is designed to be a multithreaded system. It achieves this goal in two
fundamental ways: It has a *pre-emptive*, *round-robin* scheduler, and is
designed to be a *symmetric multiprocessing (SMP)* system, which means that it
supports multiple CPU cores, running each either own thread, while sharing the
same physical memory.
