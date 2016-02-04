/*
 * Internal Threading
 */

#ifndef __INTERNAL_THREAD_H__
#define __INTERNAL_THREAD_H__

/* Includes */
#include "lib/types.h"
#include "vm/memory.h"

/* Struct */
typedef struct _kthread
{
  /* PADDING */
  uint32_t padding[5];

} _kthread_t;

#endif
