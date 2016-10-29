/*
 * Internal interrupt rutines (arch-specific)
 */

#ifndef KUDOS_KERNEL_MIPS32_IRQ_H
#define KUDOS_KERNEL_MIPS32_IRQ_H

#include "lib/types.h"
#include "drivers/device.h"

/* CAUSE register bits */

#define INTERRUPT_CAUSE_SOFTWARE_0 (1<<8)
#define INTERRUPT_CAUSE_SOFTWARE_1 (1<<9)
#define INTERRUPT_CAUSE_HARDWARE_0 (1<<10)
#define INTERRUPT_CAUSE_HARDWARE_1 (1<<11)
#define INTERRUPT_CAUSE_HARDWARE_2 (1<<12)
#define INTERRUPT_CAUSE_HARDWARE_3 (1<<13)
#define INTERRUPT_CAUSE_HARDWARE_4 (1<<14)
#define INTERRUPT_CAUSE_HARDWARE_5 (1<<15)


#define INTERRUPT_MASK_MASTER 0x1
#define INTERRUPT_MASK_SOFTWARE_0 INTERRUPT_CAUSE_SOFTWARE_0
#define INTERRUPT_MASK_SOFTWARE_1 INTERRUPT_CAUSE_SOFTWARE_1
#define INTERRUPT_MASK_HARDWARE_0 INTERRUPT_CAUSE_HARDWARE_0
#define INTERRUPT_MASK_HARDWARE_1 INTERRUPT_CAUSE_HARDWARE_1
#define INTERRUPT_MASK_HARDWARE_2 INTERRUPT_CAUSE_HARDWARE_2
#define INTERRUPT_MASK_HARDWARE_3 INTERRUPT_CAUSE_HARDWARE_3
#define INTERRUPT_MASK_HARDWARE_4 INTERRUPT_CAUSE_HARDWARE_4
#define INTERRUPT_MASK_HARDWARE_5 INTERRUPT_CAUSE_HARDWARE_5
#define INTERRUPT_MASK_ALL 0xff00
#define INTERRUPT_MASK_SOFTWARE 0x0300
#define INTERRUPT_MASK_HARDWARE 0xfc00

/* data types */
typedef uint32_t interrupt_status_t;

/* Interrupt type */
typedef void (*int_handler_t)(device_t*);

/* Arch Specific */
void _interrupt_clear_bootstrap(void);
void _interrupt_clear_sw(void);
void _interrupt_clear_sw0(void);
void _interrupt_clear_sw1(void);

void _interrupt_set_EXL(void);
void _interrupt_clear_EXL(void);

#endif // KUDOS_KERNEL_MIPS32_IRQ_H
