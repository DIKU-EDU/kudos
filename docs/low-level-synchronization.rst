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

If you would like to keep things simple, YAMS *can* be configured to run with
just 1 CPU core. See the :doc:`appendix` for more details. We recommend to get
started with spinlocks right away.

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

MIPS' Read-Modify-Write
^^^^^^^^^^^^^^^^^^^^^^^

MIPS does not provide a test-and-set instruction, but instead provides a pair
of instructions for implementing a read-modify-write (RMW) sequence. A RMW
sequence operates without any guarantee of atomicity, but is guaranteed to
succeed only if it *was* atomic. This is deliberate, as a RMW sequence scales
well with every-larger multiprocessors, and an ever-more distant shared memory,
while an atomic test-and-set instruction does not.

The ``ll`` (load-linked) instruction loads a word from the specified memory
address, and marks the beginning of a RMW sequence for that processor, on that
particular address. The RMW sequence is "broken" if a memory write to the
``ll`` address is performed by *any* processor. A processor may be in the
middle of at most one RMW sequence.

If the RMW sequence was not broken, the ``sc`` (store-conditional) instruction
will store a register value to a given address given (preferably, the ``ll``
address, otherwise, behaviour is undefined) and set the register to 1. If the
RMW sequence was broken, ``sc`` will not write to memory and instead set the
register to 0. ``sc`` overwrites the given register in either case.

The RMW sequence is pessimistic, and may fail even if no other processor has
written to the RMW address in the mean-while. For instance, it will fail if the
processor was interrupted, or if another processor has written to the same
cache line.

For an implementation of an RMW sequence in MIPS32, see
``kudos/mips32/_spinlock.S``.

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
  | MIPS32                                                |
  +=======================================================+
  |  1. ``ll`` the ``slock`` address.                     |
  |  2. If value is not 0 (free), go to step 1.           |
  |  3. Set value to 1 (taken), and ``sc`` to ``slock``.  |
  |  4. If ``sc`` failed, go to step 1.                   |
  +-------------------------------------------------------+

``void spinlock_release(spinlock_t *slock)``
  Free the specified spinlock. This does not check whether the spinlock is
  actually held by the processor. In general, there is no way to check this,
  and so requires a strict programming practice: Spinlocks should only be
  "released" if acquired.

  +---------------------------+
  | MIPS32                    |
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
