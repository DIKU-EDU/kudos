/*
 * Internal Threading
 */

#ifndef __INTERNAL_THREAD_H__
#define __INTERNAL_THREAD_H__

/* Includes */
#include "lib/types.h"
#include "vm/memory.h"

/* Defines */
typedef struct _kthread
{
    /* pad to 64 bytes */
    uint32_t dummy_alignment_fill[8];

} _kthread_t;

#endif
