/*
 * Irq Assembly Rutines
 */
.code64

/* Function Definitions */
.global yield
//.extern _threadswitch

yield:
	/* Save registers */

	/* Call switch */

	/* Switch stack */

	/* Restore registers */
	cli
	hlt
	iretq
