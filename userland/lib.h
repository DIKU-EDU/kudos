/*
 * System calls.
 *
 * Copyright (C) 2003-2014 Juha Aatrokoski, Timo Lilja,
 *   Leena Salmela, Teemu Takanen, Aleksi Virtanen,
 *   Philip Meulengracht.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: lib.h,v 1.4 2004/02/18 15:14:43 ttakanen Exp $
 *
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
