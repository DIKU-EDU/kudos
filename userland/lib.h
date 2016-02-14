/*
 * Userland library
 */

#ifndef KUDOS_USERLAND_LIB_H
#define KUDOS_USERLAND_LIB_H

/* Some library functions are quite big, and will make programs
   similarly large.  This can cause trouble, as they have to fit in
   the TLB (unless you have finished the TLB handling).  Unneeded
   portions of the library can be disabled here. */

#define PROVIDE_STRING_FUNCTIONS
#define PROVIDE_BASIC_IO
#define PROVIDE_FORMATTED_OUTPUT
#define PROVIDE_HEAP_ALLOCATOR
#define PROVIDE_MISC

#include "lib/types.h"

#define MIN(arg1,arg2) ((arg1) > (arg2) ? (arg2) : (arg1))
#define MAX(arg1,arg2) ((arg1) > (arg2) ? (arg1) : (arg2))
#define NULL ((void*)0)

typedef uint8_t byte;

/* POSIX-like integer types */

typedef intptr_t ssize_t;
typedef uintptr_t size_t;
typedef int pid_t;

/* Default initial userland buffer size */
#define BUFSIZE 64

/* Makes the syscall 'syscall_num' with the arguments 'a1', 'a2' and 'a3'. */
uint32_t _syscall(uint32_t syscall_num, uint32_t a1, uint32_t a2, uint32_t a3);

/* The library functions which are just wrappers to the _syscall function. */

void syscall_halt(void);

int syscall_spawn(const char *filename, const char **argv);
int syscall_join(int pid);
void syscall_exit(int retval);

int syscall_open(const char *filename);
int syscall_close(int filehandle);
int syscall_seek(int filehandle, int offset);
int syscall_read(int filehandle, void *buffer, int length);
int syscall_write(int filehandle, const void *buffer, int length);
int syscall_create(const char *filename, int size);
int syscall_delete(const char *filename);
int syscall_filecount(const char *pathname);
int syscall_file(const char *pathname, int index, char *buffer);

int syscall_fork(void (*func)(int), int arg);
void *syscall_memlimit(void *heap_end);

#ifdef PROVIDE_STRING_FUNCTIONS
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strstr(const char *s1, const char *s2);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
#endif

#ifdef PROVIDE_BASIC_IO
int putc(char c);
int puts(const char* s);
int getchar(void);
ssize_t gets(char *s, size_t size);
ssize_t readline_static(char *s, size_t size);
char *readline(char *prompt);
#endif

#ifdef PROVIDE_FORMATTED_OUTPUT
int printf(const char *, ...);
int snprintf(char *, int, const char *, ...);
#endif

#ifdef PROVIDE_HEAP_ALLOCATOR
#define HEAP_SIZE 256 /* 256 byte heap - puny! */
void heap_init();
void *calloc(size_t nmemb, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
#endif

#ifdef PROVIDE_MISC
int atoi(const char *nptr);
#endif

#endif /* KUDOS_USERLAND_LIB_H */
