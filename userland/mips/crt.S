/*
 * The CRT (C Run Time) assembler code for KUdos binaries 
 * (entry point is _start).
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
 * $Id: crt.S,v 1.7 2007/01/17 10:41:26 ttakanen Exp $
 *
 */

#include "proc/syscall.h"
#include "kernel/mips/asm.h"

        .text
	.align	2
	.globl	_start
	.ent	_start

_start:
	lw	$4,4(sp) /* Copy argc from stack */
	lw	$5,0(sp) /* Copy argv from stack */
	jal	main             # call main()
        nop
        addu    a1, v0, zero     # and call syscall exit with
        li      a0, SYSCALL_EXIT # the return value
        syscall
        .end    _start

        .globl  __main
        .ent    __main

/* GCC calls __main() in the beginning of main() for C++ global variable
   initialization. Some GCC versions require this function to exist
   also for C programs. */
__main:
        jr      ra
        nop
        .end    __main
