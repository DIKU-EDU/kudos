/*
 * Kernel boot arguments.
 */

#include <lib/libc.h>
#include <drivers/device.h>
#include "kernel/stalloc.h"
#include "kernel/config.h"

/** @name Kernel boot arguments
 *
 *  This module implements kernel boot argument handling. YAMS
 *  will store boot arguments to a special memory address for
 *  the kernel. Initialization function will fetch this string
 *  and parse it into (key, value) pairs. The arguments can
 *  be accessed with a key.
 *
 *  Boot arguments are given in form "key1=value1 key2=value2".
 *   @{
 */

/**
 * Table for storing the boot argument strings. The length of the table
 * is fixed.
 *
 */

static struct bootargs_t {
  char *key;
  char *value;
} bootargs_values[CONFIG_BOOTARGS_MAX];


static void bmemcopy(void *void_target, const void *void_source, int len)
{
  uint8_t *target = (uint8_t*)void_target;
  const uint8_t *source = (const uint8_t*)void_source;

  while(len-- > 0) {
    *(target++) = *(source++);
  }
}

/**
 * Initializes the boot argument system. Reads boot arguments from
 * YAMS defined memory address.
 *
 */

void bootargs_init(void *bootargs)
{
  int l,i,o,last;
  char *bootarg_area = (char *)bootargs;
  char *value_area = NULL;

  for(i=0; i<CONFIG_BOOTARGS_MAX; i++) {
    bootargs_values[i].key = NULL;
  }

  if(bootargs == NULL)
    return;

  l = strlen(bootarg_area);
  value_area = (char *) stalloc(l+1);
  bmemcopy(value_area, bootarg_area, l+1);

  i=0; o=0; last=0;

  while(o<=l) {
    /* search for = or end of key (space or 0) */
    if(*(value_area+o) == ' ' ||
       *(value_area+o) == 0   ||
       *(value_area+o) == '=') {

      bootargs_values[i].key   = value_area+last;
      bootargs_values[i].value = "";

      last = o+1;

      if(*(value_area+o) == '=') {
        *(value_area+o) = 0;
        /* we have value for this key */
        o++;
        while(o<=l) {
          if(*(value_area+o) == ' ' ||
             *(value_area+o) == 0) {
            /* value ends */
            bootargs_values[i].value = value_area+last;
            *(value_area+o) = 0;
            o++;
            last=o;
            break;
          }
          o++;
        }
      } else {
        *(value_area+o) = 0;
        o++;
      }

      i++;
    } else {
      o++;
    }
  }
}

/**
 * Gets specified boot argument.
 *
 * @param key The key to search for.
 *
 * @return Value of the key. If key is not found, NULL is returned.
 *
 */

char *bootargs_get(char *key)
{
  int i,o;

  for(i=0; i<CONFIG_BOOTARGS_MAX; i++) {
    if(bootargs_values[i].key == NULL)
      return NULL;

    o=0;
    while(*(bootargs_values[i].key+o) ==
          *(key+o)) {
      if(*(key+o) == 0)
        return bootargs_values[i].value;

      o++;
    }
  }

  return NULL;
}

/** @} */
