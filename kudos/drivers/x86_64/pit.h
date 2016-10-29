/*
 * The programmable interrupt timer
 */

#ifndef KUDOS_DRIVERS_X86_64_PIT_H
#define KUDOS_DRIVERS_X86_64_PIT_H

/* Includes */
#include "lib/types.h"

/* Defines */
#define PIT_COUNTER0_REG        0x40
#define PIT_COUNTER1_REG        0x41
#define PIT_COUNTER2_REG        0x42
#define PIT_COMMAND_REG         0x43

/* PIT Control Word Bits */
#define PIT_CW_MASK_BINCOUNT    0x1
#define PIT_CW_MASK_MODE        0xE
#define PIT_CW_MASK_RL          0x30
#define PIT_CW_MASK_COUNTER     0xC0


/* Bit 0, Binary Counter Format */
#define PIT_CW_MASK_BINARY      0
#define PIT_CW_MASK_BCD         1

/* Bit 1 - 3 Operation Mode */
#define PIT_CW_MASK_COUNTDOWN   0
#define PIT_CW_MASK_ONESHOT     2
#define PIT_CW_MASK_RATEGEN     4
#define PIT_CW_MASK_SQRRATEGEN  6
#define PIT_CW_MASK_SWTRIGGER   8
#define PIT_CW_MASK_HWTRIGGER   10

/* Bits 4 - 5 Read/Load Mode */
#define PIT_CW_MASK_LATCH       0
#define PIT_CW_MASK_LSB         0x10
#define PIT_CW_MASK_MSB         0x20
#define PIT_CW_MASK_DATA        0x30


/* Bits 6 - 7 Counter Selection */
#define PIT_CW_MASK_COUNTER0    0
#define PIT_CW_MASK_COUNTER1    0x40
#define PIT_CW_MASK_COUNTER2    0x80
#define PIT_CW_MASK_COUNTERINV  0x90

#define PIT_BASE_FREQUENCY      1193181 /* Divide this with the wished frequency */
#define PIT_FREQUENCY           100 /* 100 Interrupts a second */

/* Prototypes */
void pit_init();
uint32_t get_clock(void);
void pit_sleepms(uint64_t ms);


#endif // KUDOS_DRIVERS_X86_64_PIT_H
