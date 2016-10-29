/*
 * TTY driver for x86
 */

#ifndef KUDOS_DRIVERS_X86_64_TTY_H
#define KUDOS_DRIVERS_X86_64_TTY_H

#include <arch.h>
#include "kernel/spinlock.h"

/* x86 TTY definitions */
#define VIDEO_MEMORY_BASE       0xB8000
#define VIDEO_MEMORY_COLUMNS    80
#define VIDEO_MEMORY_ROWS       25

/* Text Colors */
#define COLOR_BLACK             0x00
#define COLOR_BLUE              0x01
#define COLOR_GREEN             0x02
#define COLOR_CYAN              0x03
#define COLOR_RED               0x04
#define COLOR_MAGENTA           0x05
#define COLOR_BROWN             0x06
#define COLOR_LGRAY             0x07
#define COLOR_DGRAY             0x08
#define COLOR_LBLUE             0x09
#define COLOR_LGREEN            0x0A
#define COLOR_LCYAN             0x0B
#define COLOR_LRED              0x0C
#define COLOR_LMAGENTA          0x0D
#define COLOR_YELLOW            0x0E
#define COLOR_WHITE             0x0F

/* The structure of a "terminal" */
typedef struct {
    uint64_t cursor_x;    /* Text position X */
    uint64_t cursor_y;     /* Text position Y */
    uint64_t textcolor;    /* Text color */
    spinlock_t *slock;    /* Synchronization */
} tty_t;

#endif // KUDOS_DRIVERS_X86_64_TTY_H
