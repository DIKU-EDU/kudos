/*
 * Kernel configuration options.
 */

#ifndef KUDOS_KERNEL_CONFIG_H
#define KUDOS_KERNEL_CONFIG_H

/* Define the maximum number of threads supported by the kernel 
 * Range from 2 (idle + init) to 256 (ASID size)
 */
#define CONFIG_MAX_THREADS 32

/* Size of the stack of a kernel thread */
#define CONFIG_THREAD_STACKSIZE 4096

/* Define the maximum number of CPUs supported by the kernel
 * Range from 1 to 32
 * CONFIG_MAX_THREADS should be the same or greater
 */
#define CONFIG_MAX_CPUS 4

/* Define the length of scheduling interval (timeslice) in 
 * processor cycles. 
 * Range from 200 to 2000000000.
 */
#define CONFIG_SCHEDULER_TIMESLICE 750

/* Sets the maximum number of boot arguments that the kernel will 
 * accept.
 * Range from 1 to 1024
 */ 
#define CONFIG_BOOTARGS_MAX 32

/* Define the maximum number of semaphores.
 * Range from 16 to 1024
 */
#define CONFIG_MAX_SEMAPHORES 128

/* Define maximum number of devices.
 * Range from 16 to 128
 */
#define CONFIG_MAX_DEVICES 128

/* Define maximum number of mounted filesystems
 * Range from 1 to 128
 */

#define CONFIG_MAX_FILESYSTEMS 8

/* Define maximum number of open files
 * Range from 16 to 65536
 */

#define CONFIG_MAX_OPEN_FILES 512

/* Maximum number of simultaneously open sockets for POP/SOP 
 * Range from 4 to 65536
 */
#define CONFIG_MAX_OPEN_SOCKETS 64

/* Size of the POP receive queue 
 * Range from 4 to 512
 */
#define CONFIG_POP_QUEUE_SIZE 32

/* Minimum time in milliseconds that POP packets stay in the input queue
 * if nobody is interested in receiving them.
 * Range from 0 to 10000
 */
#define CONFIG_POP_QUEUE_MIN_AGE 250

/* Maximum number of network interfaces 
 * Range from 1 to 64
 */
#define CONFIG_MAX_GNDS 4

/* Defines the number of pages allocated for userland stacks.
 * Range from 1 to 1000
 */
#define CONFIG_USERLAND_STACK_SIZE 1

#endif // KUDOS_KERNEL_CONFIG_H
