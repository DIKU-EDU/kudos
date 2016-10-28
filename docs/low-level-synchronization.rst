Low-Level Synchronization
=========================

KUDOS is designed to support multiple :doc:`threads of execution <threads>`. To
avoid threads getting in the way of one another, KUDOS provides a number of
low-level synchronization primitives. These can be used to demarcate a critical
region, and ensure exclusive access to a kernel resource.

The low-level primitives can then be used in :doc:`more advanced
synchronization techniques <advanced-synchronization>`.

Interrupts
----------

KUDOS has a pre-emptive, round-robin scheduler. To prevent a thread from being
pre-empted it is sufficient to disable interrupts. On a *uniprocessor* system,
it is therefore sufficient to demarcate a critical section as follows:

1. Include ``kudos/kernel/interrupt.h`` at the top of your file.

2. Declare a variable of type ``interrupt_status_t``.

3. Set this variable to the return value of ``_interrupt_disable``.

4. Have your, preferably short-lived, critical section.

5. Call ``_interrupt_set_state`` with the state stored above, i.e. restore the
   interrupt state.

Spinlocks
---------

KUDOS is designed to be a *symmetric multiprocessing (SMP)* system, which means
that it supports multiple CPU cores, having each their own :doc:`thread of
execution <threads>`, while sharing the same physical memory. This can lead to
a whole new class of race conditions, where multiple concurrent threads access
the same memory location at (what appears to be) exactly the same time.

A spinlock is the most basic, low-level synchronization primitive for
multiprocessor systems. A spinlock is acquired by repeatedly checking the
lock's value until it is available (busy-waiting or "spinning"), and then
setting the value to taken. This requires an atomic *test-and-set* or
*read-modify-write* mechanism; the architecture specific details are covered
below.

In KUDOS, a spinlock is implemented as a signed integer, having the following
meanings:

+----------+-------------------------+
| Value    | Meaning                 |
+==========+=========================+
| 0        | Free                    |
+----------+-------------------------+
| Positive | Taken                   |
+----------+-------------------------+
| Negative | Reserved for future use |
+----------+-------------------------+

To achieve low-level interprocessor synchronization, interrupts must be
disabled *and* a spinlock must be acquired. This method **must** be used in
interrupt handlers, otherwise **other code may run before the interrupt is
fully handled**.

Attempting to acquire a spinlock with interrupts disabled completely ties up
the processor (the processor enters a busy-wait loop). To ensure fair service,
other processors should release their spinlocks, and do so as quickly as
possible. Code regions protected by spinlocks should be kept as short as
possible.

Spinlocks should not be moved around in memory, i.e. they cannot be memory
managed by the virtual memory subsystem, and so must be statically allocated in
kernel memory. This should not be a problem, as spinlocks are purely a
**kernel-level synchronization primitive**.

x86_64 Exchange and test
^^^^^^^^^^^^^^^^^^^^^^^^
The x86_64 architecture provides a instruction called ``xchg``
(exchange), which exchanges two values, either from a register
to a register or from a register to a memory location. The instruction
is always atomic, which means no other process/processor can do
the same with the same memory location, at the same time.

This mechanism can be used to implement a spinlock.
The first thing to do is to load the value 1 into a register, say ``rax``. We
can now use the instruction ``xchg`` to exchange the value
of the spinlock with the value of the register ``rax``.
Remember the spinlock is free if it equals 0 and busy if it
equals to 1. Either way the spinlock know have the value 1, since
it just exchanged its value with a register that was 1.
If the new value of the register ``rax`` equals 0, it means that
the spinlock was free before and the spinlock is now acquired.
If on the other it equals 1, then it means that it was busy, and
we have to jump back and try again.

For an implementation of a spinlock for the x86_64 architecture, see
``kudos/kernel/x86_64/spinlock.S``

Spinlock API
^^^^^^^^^^^^

The low-level assembly routines implement the architecture-independent
interface specified in ``kudos/kernel/spinlock.h``.  Recall that **interrupts
must always be disabled when a spinlock is held**, otherwise Bad Things™ will
happen.

``void spinlock_reset(spinlock_t *slock)``
  Initializes the specified spinlock to be free. Should be done before any
  processor attempts to acquire the spinlock. This is really an alias to
  ``spinlock_release``.

``void spinlock_acquire(spinlock_t *slock)``
  Acquire specified spinlock; while waiting for lock to be free, spin.

  +-------------------------------------------------------+
  | x86_64                                                |
  +=======================================================+
  |  1. Set ``rax`` to 1.                                 |
  |  2. ``xchg`` the register ``rax`` and ``slock``.      |
  |  3. Test if the value of ``rax`` is 0.                |
  |  4. If step 3 is False, go to step 2.                 |
  +-------------------------------------------------------+

``void spinlock_release(spinlock_t *slock)``
  Free the specified spinlock. This does not check whether the spinlock is
  actually held by the processor. In general, there is no way to check this,
  and so requires a strict programming practice: Spinlocks should only be
  "released" if acquired.

  +---------------------------+
  | x86_64                    |
  +===========================+
  |  1. Write 0 to ``slock``. |
  +---------------------------+

Exercises
^^^^^^^^^

1. Do we need spinlocks on a uniprocessor system?

2. Why must interrupts be disabled when acquiring and holding a spinlock?
   Consider the requirement that spinlocks should be held only for a very
   short time. Is the problem purely efficiency or will something actually
   break if a spinlock is held with interrupts enabled?
