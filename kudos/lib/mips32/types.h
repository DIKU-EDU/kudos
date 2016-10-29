/*
 * Bit field types.
 */

#ifndef KUDOS_LIB_MIPS32_TYPES_H
#define KUDOS_LIB_MIPS32_TYPES_H

/* Some handy type definitions to be used in various bitfields */


typedef unsigned char uint8_t;       /* unsigned 8-bit integer */
typedef unsigned short uint16_t;     /* unsigned 16-bit integer */
typedef unsigned int uint32_t;       /* unsigned 32-bit integer */
typedef unsigned long long uint64_t; /* unsigned 64-bit integer */

typedef signed char int8_t;       /* signed 8-bit integer */
typedef signed short int16_t;     /* signed 16-bit integer */ 
typedef signed int int32_t;       /* signed 32-bit integer */
typedef signed long long int64_t; /* signed 64-bit integer */

/* The sizes of virtual and physical addresses */
typedef uint32_t physaddr_t;
typedef uint32_t virtaddr_t;
typedef uint32_t uintptr_t;
typedef int32_t intptr_t;

#define UNUSED __attribute__ ((unused))
#endif // KUDOS_LIB_MIPS32_TYPES_H
