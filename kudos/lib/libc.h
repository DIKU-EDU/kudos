/*
 * Library routines header files for KUDOS
 */
#ifndef KUDOS_LIB_LIBC_H
#define KUDOS_LIB_LIBC_H

/* This should come with the compiler (at least GCC) */
#include <stdarg.h>
#include <stddef.h>
#include "lib/types.h"

#define MIN(arg1,arg2) ((arg1) > (arg2) ? (arg2) : (arg1))
#define MAX(arg1,arg2) ((arg1) > (arg2) ? (arg1) : (arg2))


/* Kernel print routine */
void kwrite(char *s);

/* Kernel read routine */
void kread(char *s, int len);

/* formatted printing functions */
int kprintf(const char *, ...);
int snprintf(char *, int, const char *, ...);

/* the same with va_list arguments */
int kvprintf(const char *, va_list);
int vsnprintf(char *, int, const char *, va_list);

/* Prototypes for random number generator functions */
void _set_rand_seed(uint32_t seed);
uint32_t _get_rand(uint32_t range);

/* Prototypes for string manipulation functions */
int stringcmp(const char *str1, const char *str2);
char *stringcopy(char *target, const char *source, int buflen);
int strlen(const char *str);

/* memory copy */
void memcopy(int buflen, void *target, const void *source);

/* memory set */
void memoryset(void *target, char value, int size);

/* convert string to integer */
int atoi(const char *s);

/* Byte Swapping */
uint16_t from_big_endian16(uint16_t in);
uint32_t from_big_endian32(uint32_t in);
uint16_t to_big_endian16(uint16_t in);
uint32_t to_big_endian32(uint32_t in);

uint32_t wordpad(uint32_t in);

#endif // KUDOS_LIB_LIBC_H
