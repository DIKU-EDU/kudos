/*
 * Internal Threading
 */

#ifndef KUDOS_KERNEL_X86_64__THREAD_H
#define KUDOS_KERNEL_X86_64__THREAD_H

/* Includes */
#include "lib/types.h"
#include "vm/memory.h"

/* Struct */
typedef struct _kthread
{
  /* PADDING */
  uint32_t padding[5];

} _kthread_t;

#endif // KUDOS_KERNEL_X86_64__THREAD_H
