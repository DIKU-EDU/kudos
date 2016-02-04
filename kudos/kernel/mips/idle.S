/*
 * The wait loop for the idle thread (TID 0).
 */

#include "lib/registers.h"
	
        .text
	.align	2
	.globl	_idle_thread_wait_loop
	.ent	_idle_thread_wait_loop

	# A bit overkill to put this in its own file, but...
_idle_thread_wait_loop:	
	wait     # Enter sleep mode until an interrupt occurs
	j _idle_thread_wait_loop
	
        .end    _idle_thread_wait_loop
