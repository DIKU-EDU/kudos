/*
 * Drivers
 */
#include <arch.h>
#include <tty.h>
#include "lib/libc.h"
#include "drivers/drivers.h"
#include "drivers/disk.h"
#include "drivers/metadev.h"

/**  
 * A table of available device drivers.
 */
drivers_available_t drivers_available[] = {
    {TYPECODE_TTY, "Console", &tty_init} ,
    {YAMS_TYPECODE_RTC, "System RTC", &rtc_init} ,
    {YAMS_TYPECODE_MEMINFO, "System memory information", &meminfo_init} ,
    {YAMS_TYPECODE_SHUTDOWN, "System shutdown", &shutdown_init} ,
    {YAMS_TYPECODE_CPUSTATUS, "CPU status", &cpustatus_init} ,
    {TYPECODE_DISK, "Disk", &disk_init},
    {0, NULL, NULL}
};
