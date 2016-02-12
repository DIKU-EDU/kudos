/*
 * Random numbers.
 */

#include "lib/registers.h"
        
        .data
rand5_a:       
        .word 314159557

rand5_c:
        .word 23550511
rand5_seed:
        .word 0
        
       	.text
	.align	2

        .globl  _set_rand_seed
        .ent    _set_rand_seed
_set_rand_seed:
        sw      a0, rand5_seed
        jr      ra
        .end _set_rand_seed
        
	.globl	_get_rand
	.ent	_get_rand
_get_rand:
        lw t0, rand5_a     # Initialize a                     (1)
        lw t1, rand5_c     # Initialize c
        lw t2, rand5_seed  # Load X

        multu t2, t0        # Compute a*X                      (2)
        mflo t2             # get a*X mod m (m=2^32)
        addu t2, t2, t1     # X=((a*X) mod m)+ c mod m == X=(a*X+c) mod m

        sw t2, rand5_seed   # Save X for next call             (3)

        # Compute uniformly distributed number between 0 and range-1
        # Use highest bits of X since they are "best" in terms
        # of random distribution

                             #                                  (4)
        multu t2, a0         # X*range, think X as a fraction of X and
                             # it makes much more sense. Note that
                             # X is never more than 0xffffffff so now
                             # we have number [0,range-1] in $hi.

        mfhi v0              # X=[0,range-1]
        jr      ra
 
	.end	_get_rand
