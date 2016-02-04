/*
 * Metadevices (RTC)
 */
#include <pit.h>
#include "drivers/metadev.h"
#include "lib/libc.h"
#include "kernel/panic.h"
#include "kernel/assert.h"
#include "kernel/stalloc.h"
#include "kernel/interrupt.h"

/**@name Metadevices
 *
 * This module implements drivers for so called metadevices. These are
 * devices documented in YAMS specification as non-peripheral devices.
 * Metadevices include RTC (Real Time Clock), MemInfo (Memory
 * Information) and shutdown device.
 * They are not really used for X86 and we thus only implement
 * the bare minimum for dual-architecture compatability
 * @{
 */

/** Get number of timer ticks elapsed since system startup from the
 * system RTC. Returns the number of timer ticks since system startup
 *
 * @return Number of timer ticks elapsed
 */
uint32_t rtc_get_msec()
{
    return get_clock(); /* MSEC @0x00 */
}
