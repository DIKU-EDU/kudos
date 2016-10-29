/*
 * Context switch.
 */

#ifndef KUDOS_KERNEL_INTERRUPT_H
#define KUDOS_KERNEL_INTERRUPT_H

#include "lib/types.h"
#include "drivers/device.h"
#include <irq.h>
#include "vm/memory.h"

/* structure for registered interrupt handlers */
typedef struct {
    device_t *device;
    uint32_t irq;
    int_handler_t handler;
} interrupt_entry_t;

/* C functions */

void interrupt_init(int num_cpus);
void interrupt_register(uint32_t irq, int_handler_t handler, device_t *device);
void interrupt_handle(virtaddr_t cause);

/* assembler functions */
interrupt_status_t _interrupt_disable(void);
interrupt_status_t _interrupt_enable(void);
interrupt_status_t _interrupt_set_state(interrupt_status_t);
interrupt_status_t _interrupt_get_state(void);
interrupt_status_t _interrupt_is_disabled(void);

/* Arch Specific */
void _interrupt_yield(void);
int _interrupt_getcpu(void);

#endif // KUDOS_KERNEL_INTERRUPT_H
