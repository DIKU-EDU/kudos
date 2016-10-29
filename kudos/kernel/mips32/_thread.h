/*
 * Internal Threading
 */

#ifndef KUDOS_KERNEL_MIPS32__THREAD_H
#define KUDOS_KERNEL_MIPS32__THREAD_H

/* Includes */
#include "lib/types.h"
#include "vm/memory.h"

/* Defines */
typedef struct _kthread
{
    /* pad to 64 bytes */
    uint32_t dummy_alignment_fill[8];

} _kthread_t;

#endif // KUDOS_KERNEL_MIPS32__THREAD_H
