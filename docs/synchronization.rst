Synchronization
===============

KUDOS is designed to support multiple :doc:`threads of execution <threads>`.

Interrupts
----------

KUDOS has a pre-emptive, round-robin scheduler. To prevent a thread from being
pre-empted it is sufficient to disable interrupts. On a *uniprocessor* system,
it is therefore sufficient to demarkate a critical section as follows:

1. Include ``kudos/kernel/interrupt.h`` at the top of your file.

2. Declare a variable of type ``interrupt_status_t``.

3. Set this variable to the return value of ``_interrupt_disable``.

4. Have your, preferrably short-lived, critical section.

5. Call ``_interrupt_set_state`` with the state stored above, i.e. restore the
   interrupt state.

Spinlocks
---------

KUDOS is designed to be a *symmetric multiprocessing (SMP)* system, which means
that it supports multiple CPU cores, having each their own :doc:`thread of
execution <threads>`, while sharing the same physical memory. This can lead to
a whole new class of race conditions, where multiple concurrent threads access
the same memory location at (what appears to be) exactly the same time.
