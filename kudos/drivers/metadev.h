/*
 * Metadevices (RTC, meminfo, CPUinfo, shutdown)
 */

#ifndef KUDOS_DRIVERS_METADEV_H
#define KUDOS_DRIVERS_METADEV_H

#include <arch.h>
#include "lib/types.h"
#include "drivers/device.h"
#include "kernel/spinlock.h"

#define DEFAULT_SHUTDOWN_MAGIC 0xdeadc0de
#define POWEROFF_SHUTDOWN_MAGIC 0x0badf00d

#define CPU_COMMAND_RAISE_IRQ 0x00
#define CPU_COMMAND_CLEAR_IRQ 0x01

#define CPU_STATUS_IRQ(status) \
    ((status) & 0x00000002)

/* The structure of the YAMS CPU status device IO area */
typedef struct {
    uint32_t status;   /* Status port of the CPU status device */
    uint32_t command;  /* Command port of the CPU status device */
} cpu_io_area_t;


/* The real device structure for a CPU status device. Structure of
   this type is stored the real_device field of device_t data
   structure. Currently this only provides synchronization for
   generating and clearing interrupts. */
typedef struct {
    /* Spinlock to synchronize access to the driver */
    spinlock_t slock;
    /* If you are using inter-cpu interrupts for something, the
       necessary data may be added here. */
} cpu_real_device_t;

device_t *rtc_init(io_descriptor_t *desc);
uint32_t rtc_get_msec(void);
uint32_t rtc_get_clockspeed(void);

device_t *meminfo_init(io_descriptor_t *desc);
uint32_t meminfo_get_pages(void);

device_t *cpustatus_init(io_descriptor_t *desc);
int cpustatus_count(void);
void cpustatus_generate_irq(device_t *dev);
void cpustatus_interrupt_handle(device_t *dev);

device_t *shutdown_init(io_descriptor_t *desc);
void shutdown(uint32_t magic);

#endif // KUDOS_DRIVERS_METADEV_H
