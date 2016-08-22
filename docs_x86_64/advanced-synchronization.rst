Advanced Synchronization
========================

The :doc:`low-level synchronization primitives <low-level-synchronization>`,
such as disabling interrupts and spinlocks, can be used to implement more
advanced synchronization techniques. Kernel-level KUDOS supports sleep queues
and semaphores. Documentation about kernel-level semaphores is coming soon.

Sleep Queue
-----------

As a spinlock busy-waits until the resource is free, it wastes clock-cycles.
Another synchronization method, a sleep queue, allows a thread to register
itself as waiting on a specific resource held by another thread, and then
voluntarily give up the CPU to other threads until the resource is available.
The resource is identified by an address in kernel memory, so that the same
address is used by both the thread holding the resource, and the thread waiting
on the resource. When the resource is released by the thread holding it, the
state of the thread waiting on it is updated, so that it will be eligible to
run again.

Sleep Queue API
^^^^^^^^^^^^^^^

The sleep queue API is defined in `kudos/kernel/sleepq.h`, but **should not**
be used frivolously. See the following section for notes on how to
:ref:`correctly use the sleep queue API <correct-use>`.

``void sleepq init(void)``
  Initializes the internal sleep queue data structured. Must be called on boot.

``void sleepq_add(void *resource)``
  Adds the currently running thread to the sleep queue, sleeping on the given
  resource. To maximize multithreading, threads should sleep on exactly the
  resource that they need (e.g. array element instead of en entire array).

  The thread does not go to sleep when calling this function; its ``state`` is
  simply set to ``SLEEPING``. An explicit call to ``thread_switch`` is needed.
  See the :ref:`following section <correct-use>` for details.

``void sleepq_wake(void *resource)``
  Wakes the first thread waiting for the given ``resource`` from the queue. If
  no threads are waiting for the given ``resource``, do nothing.

``void sleepq_wake_all(void *resource)``
  As ``sleepq_wake``, but wakes up all threads which are waiting on the given
  ``resource``.

.. _correct-use:

Using the Sleep Queue Correctly
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is much more to using the sleep queue than just using the sleep queue API
(defined in `kudos/kernel/sleepq.h`). To wait on a resource, you will need a
dedicated resource spinlock. For details about why all of the below steps are
necessary, see the sleep queue implementation notes below.

Sleeping on a Resource
""""""""""""""""""""""
::

  Disable interrupts
  Acquire the resource spinlock
  While we want to sleep :
    sleepq_add ( resource ) // Thread state set to SLEEPING
    Release the resource spinlock // Spinlock cannot be held when not on CPU
    thread_switch () // Voluntarily yield CPU to other threads
    Acquire the resource spinlock // Thread state must now be RUNNING
  End While
  Use the resource
  Release the resource spinlock
  Restore the interrupt mask

Disabling interrupts and acquiring the resource spinlock ensures that the
thread will be in the sleep queue before another thread attempts to wake it. If
another thread was run, which called ``sleepq_wake`` before the ``sleepq_add``
call was complete, then the resource may be free, but the first thread would
still be waiting on it!

Awaking Threads Sleeping on a Resource
""""""""""""""""""""""""""""""""""""""

::

  Disable interrupts
  Acquire the resource spinlock
  Use the resource
  If wishing to wake up something
    sleepq_wake ( resource ) or sleepq_wake_all ( resource )
  End If
  Release the resource spinlock
  Restore the interrupt mask

Implementation
^^^^^^^^^^^^^^

In KUDOS, the sleep queue is implemented as a statically-sized hashtable,
``sleepq_hashtable``. This hashtable is intimately connected with the
``sleeps_on`` and ``next`` fields of the ``thread_table_t``, which form part of
the hashtable data structure. The hashtable itself is protected from concurrent
access by multiple threads with a spinlock ``sleepq_slock``. The implementation
will acquire and release the ``thread_table_slock`` as it becomes necessary.

Interrupts must be disabled, and the ``sleepq_slock`` must be held, before any
sleep queue operations are carried out. The user however, should merely disable
interrupts, or better yet, follow the recipes given above.

Each entry in the hashtable corresponds to the hashed value of resource
addresses; the same entry may correspond to multiple resources that hash to the
same key. A value is a ``TID_t``, corresponding to the ``thread_table_t`` of
the first thread waiting on a resource with this key. The ``sleeps_on`` field
of the ``thread_table_t`` is used to store the (non-hashed) address of the
actual resource that the thread is waiting for – it is 0 if the thread is not
waiting on any resource.  This ``next`` field of the ``thread_table_t``,
contains the ``TID_t`` of the next thread waiting on a resource with this hash,
if any. New threads are added to the end of this linked list, and threads are
awoken from the beginning of the chain, to avoid potentially having to run
through the whole list. Note that that as multiple resources will have the same
hash, the first thread in the chain isn't necessarily the one awoken.

.. figure:: sleepq.svg

    An illustration of the sleep queue hashtable.

``void sleepq init(void)``
  Sets all hashtable values to -1 (free).

``void sleepq_add(void *resource)``
  Adds the currently running thread into the sleep queue. The thread is added
  to the sleep queue hashtable. The thread does not go to sleep when calling
  this function; its ``state`` is simply set to ``SLEEPING``. An explicit call
  to ``thread_switch`` is needed. The thread will sleep on the given
  ``resource`` address. Implementation:

  1. Assert that interrupts are disabled. Interrupts need to be disabled
     because the thread holds a spinlock and because otherwise the thread
     can be put to sleep by the scheduler before it is actually ready to
     do so.
  2. Set the current thread's sleeps on field to the resource.
  3. Lock the sleep queue structure.
  4. Add the thread to the queue's end by hashing the address of given
     resource.
  5. Unlock the sleep queue structure.

``void sleepq_wake(void *resource)``
  Wakes the first thread waiting for the given ``resource`` from the queue. If
  no threads are waiting for the given ``resource``, do nothing.
  Implementation:

  1. Disable interrupts.
  2. Lock the sleep queue structure.
  3. Find the first thread waiting for the given resource by hashing the
     resource address and walking through the chain.
  4. Remove the found thread from the sleep queue hashtable.
  5. Lock the thread table.
  6. Set sleeps on to zero on the found thread.
  7. If the thread is sleeping, add it to the scheduler’s ready list by calling
     scheduler add to ready list.
  8. Unlock the thread table.
  9. Unlock the sleep queue structure.
  10. Restore the interrupt mask.

``void sleepq_wake_all(void *resource)``
  As ``sleepq_wake``, but wakes up all threads which are waiting on the given
  resource.
