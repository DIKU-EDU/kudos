/*
 * Kernel panic.
 */

#ifndef KUDOS_KERNEL_ASSERT_H
#define KUDOS_KERNEL_ASSERT_H

#include "kernel/panic.h"

/* Causes kernel panic if condition is false */
#define KERNEL_ASSERT(condition) \
if(!(condition))\
_kernel_panic(__FILE__, __LINE__, "Assertion failed")

#endif // KUDOS_KERNEL_ASSERT_H
