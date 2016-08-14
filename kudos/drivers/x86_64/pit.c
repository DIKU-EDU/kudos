/*
 * The programmable interrupt timer
 */

#include <pit.h>
#include <asm.h>
#include <idt.h>
#include "kernel/interrupt.h"
#include "lib/types.h"
#include "lib/libc.h"
#include "drivers/modules.h"

/* Globals */
volatile uint32_t pit_counter = 0;

/* extern */
extern void pit_irq_handler(void);

/* Helpers */
void pit_send_data(uint8_t data, uint8_t counter)
{
  /* Get correct port */
  uint8_t port = 
    (counter == PIT_CW_MASK_COUNTER0) ? (uint8_t)PIT_COUNTER0_REG : 
    (counter == PIT_CW_MASK_COUNTER1) ? (uint8_t)PIT_COUNTER1_REG :
    (uint8_t)PIT_COUNTER2_REG;

  /* Send it */
  _outb(port, data);
}

void pit_send_command(uint8_t data)
{
  /* Send it */
  _outb(PIT_COMMAND_REG, data);
}

void pit_start_counter(uint32_t frequency, uint8_t counter, uint8_t mode)
{
  /* Sanity */
  if(frequency == 0)
    return;

  uint16_t divisor = (uint16_t)(PIT_BASE_FREQUENCY / frequency);

  /* Set command words */
  uint8_t cw = 0;
  cw |= mode;
  cw |= PIT_CW_MASK_DATA;
  cw |= counter;

  /* Send them */
  pit_send_command(cw);

  /* Set frequency */
  pit_send_data((uint8_t)(divisor & 0xFF), counter);
  pit_send_data((uint8_t)((divisor >> 8) & 0xFF), counter);
}

/* Initialises the PIT */
void pit_init()
{
  /* Install interrupt */
  interrupt_register(0, (int_handler_t)pit_irq_handler, 0);

  /* Start counter 0 */
  pit_start_counter(PIT_FREQUENCY, PIT_CW_MASK_COUNTER0, PIT_CW_MASK_RATEGEN);
}

uint32_t get_clock(void)
{
  return pit_counter;
}

void __attribute__((noinline)) pit_sleepms(uint64_t ms)
{
  /* Make sure interrupts are enabled */
  interrupt_status_t intr_status;
  intr_status = _interrupt_enable();

  uint32_t clocks = get_clock();
  uint32_t ticks = (uint32_t)(ms / (1024 / PIT_FREQUENCY)) + clocks;

  while(1)
    {
      if(clocks > ticks)
        break;

      clocks = get_clock();

      /* hlt */
      asm volatile("pause");
    }

  _interrupt_set_state(intr_status);
}
