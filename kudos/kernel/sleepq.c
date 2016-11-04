/*
 * Sleep queue
 */

#include "kernel/sleepq.h"
#include "kernel/thread.h"
#include "kernel/klock.h"
#include "kernel/spinlock.h"
#include "kernel/config.h"
#include "kernel/interrupt.h"
#include "kernel/assert.h"
#include "vm/memory.h"

/** @name Sleep queue
 *
 * The sleep queue is the mechanism which allows threads to go to
 * sleep while waiting on a resource to become available and be woken
 * once said resource does become available. A thread going to sleep
 * waiting for a resource (usually a memory address) is placed in a
 * hash table, from which it can be quickly found and awakened once
 * the resource becomes available.
 *
 * The resources are referenced by memory address. The address is used
 * only as a key, it is never referenced by the sleep queue mechanism.
 *
 * @{
 */

/* Size of the sleep queue hashtable (prime number) */
#define SLEEPQ_HASHTABLE_SIZE 127

extern thread_table_t thread_table[CONFIG_MAX_THREADS];
extern klock_t thread_table_klock;

/* spinlock for synchronizing sleep queue table access */
static spinlock_t sleepq_slock;
/* the sleep queue hashtable itself */
static TID_t sleepq_hashtable[SLEEPQ_HASHTABLE_SIZE];


/* Hash function used to index the sleep queue table */
#define SLEEPQ_HASH(res) ((virtaddr_t)(res) % SLEEPQ_HASHTABLE_SIZE)

/** Initializes the sleep queue system. The hashtable entries are all
 * set to -1 (NULL) and the spinlock is reset (set to 0=free).
 */
void sleepq_init(void)
{
  int i;

  for (i=0; i<SLEEPQ_HASHTABLE_SIZE; i++) {
    sleepq_hashtable[i] = -1;
  }

  spinlock_reset(&sleepq_slock);
}

/** Adds the currently running thread into the sleep queue. The thread
 * is added to the hash table and it is marked as waiting for the
 * specified resource. This function does not cause the thread to go
 * to sleep, the thread must switch explicitly after calling this
 * function. Before switching, the thread usually frees the resource
 * it will start waiting for (release some spinlock).
 * 
 * Note that interrupts must be disabled before calling this function.
 *
 * @param resource The resource to wait for
 */
void sleepq_add(void *resource)
{
  uint32_t hash;
  TID_t my_tid;

  /* Interrupts _must_ be disabled when calling this function: */
  if(!_interrupt_is_disabled())
    return;

  hash = SLEEPQ_HASH(resource);
  my_tid = thread_get_current_thread();
  /* the thread to be added should not have a next entry: */
  thread_table[my_tid].next = -1;
  thread_table[my_tid].sleeps_on = (virtaddr_t)resource; 

  /* Idle thread should never do _anything_ (other than its own wait loop) */
  KERNEL_ASSERT(my_tid != IDLE_THREAD_TID);

  spinlock_acquire(&sleepq_slock);

  /* Add the current thread to the end of the sleepqueue */
  if (sleepq_hashtable[hash] <= 0) {
    /* hashtable entry empty */
    sleepq_hashtable[hash] = my_tid;
  } else {
    TID_t prev;
    /* hashtable entry nonempty, chain to end of linked list */
    prev = sleepq_hashtable[hash];
    while (thread_table[prev].next > 0) {
      prev = thread_table[prev].next;
    }
    thread_table[prev].next = my_tid;
  }

  spinlock_release(&sleepq_slock);
}

/* Import prototype for unsafe function from scheduler.c */
void scheduler_add_to_ready_list(TID_t t);


/** Wake the first thread waiting for given resource from the sleep
 * queue. If such a thread exists, it is removed from the sleep queue
 * and placed on the scheduler's ready-to-run list.
 *
 * @param resource Wake the first thread waiting for this resource
 */
void sleepq_wake(void *resource)
{
  uint32_t hash;
  interrupt_status_t intr_state;
  TID_t first, prev;

  hash = SLEEPQ_HASH(resource);

  intr_state = _interrupt_disable();
  spinlock_acquire(&sleepq_slock);

  /* Find the first entry actually waiting for 'resource', since
   * multiple resources may hash to the same index. 
   */
  prev = -1;
  first = sleepq_hashtable[hash];
  while (first > 0 && thread_table[first].sleeps_on != (virtaddr_t)resource) {
    prev = first;
    first = thread_table[first].next;
  }

  /* First entry with correct resource found */
  if (first > 0) {
    /* remove it from the sleep queue */
    if (prev <= 0) { 
      /* it was the first entry in the table slot */
      sleepq_hashtable[hash] = thread_table[first].next;
    } else {
      thread_table[prev].next = thread_table[first].next;
    }

    /* Clear the sleeps_on field and add the thread to the ready
     * list (if necessary)
     */
    spinlock_acquire(&thread_table_klock);

    thread_table[first].sleeps_on = 0;
    thread_table[first].next = -1;
        
    if (thread_table[first].state == THREAD_SLEEPING) {
      thread_table[first].state = THREAD_READY;
      scheduler_add_to_ready_list(first);
    }

    spinlock_release(&thread_table_klock);
  }

  spinlock_release(&sleepq_slock);
  _interrupt_set_state(intr_state);
}


/** Wake all threads waiting for given resource from the sleep
 * queue. If such threads exists, they are removed from the sleep
 * queue and placed on the scheduler's ready-to-run list.
 *
 * @param resource Wake threads waiting for this resource
 */
void sleepq_wake_all(void *resource)
{
  uint32_t hash;
  interrupt_status_t intr_state;
  TID_t first, prev, wake;

  hash = SLEEPQ_HASH(resource);

  intr_state = _interrupt_disable();
  spinlock_acquire(&sleepq_slock);

  /* init linked list traversing variables to the first item */
  prev = -1;
  first = sleepq_hashtable[hash];

  /* Traverse the whole linked list in order to wake up all threads */
  while (first > 0) {

    /* Find the next entry actually waiting for 'resource', since
     * multiple resources may hash to the same index. 
     */
    while (first > 0 
           && thread_table[first].sleeps_on != (virtaddr_t)resource) {
      prev = first;
      first = thread_table[first].next;
    }

    /* Next entry w/ resource found */
    if (first > 0) {
      wake = first;
      /* remove it from the sleep queue */
      if (prev <= 0) { 
        /* it was the first entry in the table slot */
        first = sleepq_hashtable[hash] = thread_table[wake].next;
      } else {
        first = thread_table[prev].next = thread_table[wake].next;
      }

      /* Clear the sleeps_on field and add the thread to the ready
       * list (if necessary)
       */
      spinlock_acquire(&thread_table_klock);

      thread_table[wake].sleeps_on = 0;
      thread_table[wake].next      = -1;
        
      if (thread_table[wake].state == THREAD_SLEEPING) {
        thread_table[wake].state = THREAD_READY;
        scheduler_add_to_ready_list(wake);
      }

      spinlock_release(&thread_table_klock);
    }
  }

  spinlock_release(&sleepq_slock);
  _interrupt_set_state(intr_state);
}

/** @} */
