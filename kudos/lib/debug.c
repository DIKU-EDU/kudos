/*
 * Conditional debug printing.
 */

#include "lib/debug.h"
#include "drivers/bootargs.h"

/**
 * Print given debug message as with printf if debug level matches.
 *
 * @param debuglevelname If this same string has been given as
 * a boot argument to kernel, the debug message is printed.
 *
 * @param format (and ...) Format string as for printf.
 *
 */

void DEBUG(char *debuglevelname, char *format, ...)
{
  if(bootargs_get(debuglevelname) != NULL) {
    va_list args;
    va_start(args, format);

    kvprintf(format, args);

    va_end(args);
  }
}


