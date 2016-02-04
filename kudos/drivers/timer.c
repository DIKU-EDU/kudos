/*
 * timer handling
 */

#include "lib/types.h"
#include "kernel/interrupt.h"

/**
 * This module implements Co-processor 0 timer driver.
 *
 * @{
 */

/* import assembler function for clock handling */
extern void _timer_set_ticks(uint32_t ticks);

/**
 * Sets timer interrupt (hw interrupt 5) to fire after ticks.
 *
 * @param ticks Number of ticks until next timer interrupt.
 *
 */

void timer_set_ticks(uint32_t ticks)
{
    interrupt_status_t intr_status;

    intr_status = _interrupt_disable();
    _timer_set_ticks(ticks);
    _interrupt_set_state(intr_status);
}

/** @} */
