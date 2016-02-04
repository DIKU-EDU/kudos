/*
 * Threading system.
 */

#include "lib/libc.h"
#include "kernel/spinlock.h"
#include "kernel/thread.h"
#include "kernel/scheduler.h"
#include "kernel/panic.h"
#include "kernel/assert.h"
#include "kernel/config.h"
#include "kernel/interrupt.h"
#include "kernel/idle.h"

/** @name Thread library
 *
 * Library containing thread creation, control and destruction functions.
 *
 * @{
 */

/** Spinlock which must be held when manipulating the thread table */
spinlock_t thread_table_slock;

/** The table containing all threads in the system, whether active or not. */
thread_table_t thread_table[CONFIG_MAX_THREADS];

/* Thread stack areas for kernel threads */
char thread_stack_areas[CONFIG_THREAD_STACKSIZE * CONFIG_MAX_THREADS];

/* Import running thread id table from scheduler */
extern TID_t scheduler_current_thread[CONFIG_MAX_CPUS];

/** Initializes the threading system. Does this by setting all thread
 *  table entry states to THREAD_FREE. Called only once before any
 *  threads are created.
 */
void thread_table_init(void)
{
  int i;

  /* Thread table entry _must_ be 64 bytes long, because the
     context switching code in kernel/cswitch.S expects that. Let's
     make sure it is. If you hit this error, you have changed either
     context_t or thread_table_t, but have not changed padding in
     the end of thread_table_t definition in kernel/thread.h */
  KERNEL_ASSERT(sizeof(thread_table_t) == 64);

  spinlock_reset(&thread_table_slock);

  /* Init all entries to 'NULL' */
  for (i=0; i<CONFIG_MAX_THREADS; i++) {
    /* Set context pointers to the top of the stack*/
    thread_table[i].context      = (context_t *) (thread_stack_areas
                                                  +CONFIG_THREAD_STACKSIZE*i + CONFIG_THREAD_STACKSIZE -
                                                  sizeof(context_t));
    thread_table[i].user_context = NULL;
    thread_table[i].state        = THREAD_FREE;
    thread_table[i].sleeps_on    = 0;
    thread_table[i].pagetable    = NULL;
    thread_table[i].attribs      = 0;
    thread_table[i].process_id   = -1;
    thread_table[i].next         = -1;
  }

  /* Setup Idle Thread */
  _context_set_ip(thread_table[IDLE_THREAD_TID].context,
                  (virtaddr_t)_idle_thread_wait_loop);
  _context_set_sp(thread_table[IDLE_THREAD_TID].context,
                  (virtaddr_t) thread_stack_areas + CONFIG_THREAD_STACKSIZE - 4 -
                  sizeof(context_t));
  _context_enable_ints(thread_table[IDLE_THREAD_TID].context);

  thread_table[IDLE_THREAD_TID].state = THREAD_READY;
  thread_table[IDLE_THREAD_TID].context->prev_context =
    thread_table[IDLE_THREAD_TID].context;
}


/** Creates a new thread. A free slot is allocated from the thread
 * table for the new thread and its content is initialized to 'nil'
 * values. The new thread will call function 'func' with the argument
 * 'arg' when the thread is run by thread_run().
 *
 * @param func Function pointer to the threads 'main' function.
 * @param arg Argument to pass to 'func' (meaning defined by 'func').
 *
 * @return The thread ID of the created thread, or negative if
 * creation failed (thread table is full).
 */
TID_t thread_create(void (*func)(uint32_t), uint32_t arg)
{
  static TID_t next_tid = 0;
  TID_t i, tid = -1;
  func = func;
  arg = arg;

  interrupt_status_t intr_status;

  intr_status = _interrupt_disable();

  spinlock_acquire(&thread_table_slock);

  /* Find the first free thread table entry starting from 'next_tid' */
  for (i=0; i<CONFIG_MAX_THREADS; i++) {
    TID_t t = (i + next_tid) % CONFIG_MAX_THREADS;

    if(t == IDLE_THREAD_TID)
      continue;

    if (thread_table[t].state
        == THREAD_FREE) {
      tid = t;
      break;
    }
  }

  /* Is the thread table full? */
  if (tid < 0) {
    spinlock_release(&thread_table_slock);
    _interrupt_set_state(intr_status);
    return tid;
  }

  next_tid = (tid+1) % CONFIG_MAX_THREADS;

  thread_table[tid].state = THREAD_NONREADY;

  spinlock_release(&thread_table_slock);
  _interrupt_set_state(intr_status);

  thread_table[tid].context      = (context_t *) (thread_stack_areas
                                                  +CONFIG_THREAD_STACKSIZE*tid + CONFIG_THREAD_STACKSIZE -
                                                  sizeof(context_t));

  for (i=0; i< (int) sizeof(context_t)/4; i++) {
    *(((virtaddr_t*) thread_table[tid].context) + i) = 0;
  }

  thread_table[tid].user_context = NULL;
  thread_table[tid].pagetable    = NULL;
  thread_table[tid].sleeps_on    = 0;
  thread_table[tid].attribs      = 0;
  thread_table[tid].process_id   = -1;
  thread_table[tid].next         = -1;

  /* Make sure that we always have a valid back reference on context chain */
  thread_table[tid].context->prev_context = thread_table[tid].context;

  /* This functions magically sets up context to new
   * working state */
  _context_init(thread_table[tid].context, (virtaddr_t)func,
                (virtaddr_t)thread_finish,
                (virtaddr_t)thread_stack_areas + (CONFIG_THREAD_STACKSIZE * tid)
                + CONFIG_THREAD_STACKSIZE - 4 - sizeof(context_t),
                arg);

  return tid;
}


/** Run a thread. The given thread is added to the scheduler's
 * ready-to-run list. This is really just a wrapper for
 * scheduler_add_ready().
 *
 * @param t The ID of the thread to be run.
 */
void thread_run(TID_t t)
{
  scheduler_add_ready(t);
}



/** Perform voluntary rescheduling. The current (=calling) thread will
 * voluntary end its time slice when this function is called. The
 * thread will remain in a ready state, and if there are no other
 * threads which are ready to run, the calling thread will resume its
 * execution immediately. This should NOT be used as a substitute for
 * sleeping.
 */
void thread_switch(void)
{
  interrupt_status_t intr_status;

  intr_status = _interrupt_enable();
  _interrupt_yield();
  _interrupt_set_state(intr_status);
}

/**
 * Return the TID of the calling thread.
 * Finds out what is the TID of the thread calling this function.
 *
 * @return Thread ID of the calling thread.
 */

TID_t thread_get_current_thread(void)
{
  TID_t t;
  interrupt_status_t intr_status;

  intr_status = _interrupt_disable();

  t = scheduler_current_thread[_interrupt_getcpu()];

  _interrupt_set_state(intr_status);

  return t;
}

/**
 * Return the thread table entry of thread with given TID.
 *
 * @return Thread entry in thread table for the thread. Note that the
 * returned value is the actual entry and all acccess synchronizations
 * must be handled by caller of this function.
 */

thread_table_t *thread_get_thread_entry(TID_t tid)
{
  return &thread_table[tid];
}

/**
 * Return the thread table entry of current thread.
 *
 * @return Thread entry in thread table for current thread. Note that
 * the returned value is the actual entry and all acccess
 * synchronizations must be handled by caller of this function.
 */

thread_table_t *thread_get_current_thread_entry(void)
{
  TID_t t;
  interrupt_status_t intr_status;

  intr_status = _interrupt_disable();

  t = scheduler_current_thread[_interrupt_getcpu()];

  _interrupt_set_state(intr_status);

  return thread_get_thread_entry(t);
}

/**
 * Changes the calling thread to userland thread. This function
 * will never return.
 *
 * @param usercontext Context in userland into which this thread will
 * enter.
 *
 */

void thread_goto_userland(context_t *usercontext)
{
  /* Call platform-specific */
  _context_enter_userland(usercontext);

  KERNEL_PANIC("Userland entering returned for unknown reason.");
}

/** Perform suicide. The calling thread will kill itself by freeing
 * its memory and other resources and marking itself as dying. The
 * scheduler will free the thread table entry when it encounters dying
 * threads.
 */
void thread_finish(void)
{
  TID_t my_tid;

  my_tid = thread_get_current_thread();

  _interrupt_disable();

  /* Check that the page mappings have been cleared. */
  KERNEL_ASSERT(thread_table[my_tid].pagetable == NULL);

  spinlock_acquire(&thread_table_slock);
  thread_table[my_tid].state = THREAD_DYING;
  spinlock_release(&thread_table_slock);

  _interrupt_enable();
  _interrupt_yield();

  /* not possible without a stack? alternative in assembler? */
  KERNEL_PANIC("thread_finish(): thread was not destroyed");
}

/** @} */
