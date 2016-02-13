/* yams  -- Yet Another Machine Simulator
   Copyright (C) 2002-2005 Juha Aatrokoski, Timo Lilja, Leena Salmela,
   Teemu Takanen, Aleksi Virtanen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: cfg.c,v 1.27 2010/11/24 13:33:20 jaatroko Exp $

   $Log: cfg.c,v $
   Revision 1.27  2010/11/24 13:33:20  jaatroko
   replace little-endian command line option with a configuration file option

   Revision 1.26  2005/06/05 14:19:31  jaatroko
   support for plugin configuration

   Revision 1.25  2005/04/16 11:03:21  jaatroko
   Removed all references to alloca() since it was not used

   Revision 1.24  2005/03/08 12:57:01  lsalmela
   Inter-cpu interrupts

   Revision 1.23  2003/05/15 13:42:47  jaatroko
   missing exit(1) in disk creation

   Revision 1.22  2003/05/13 15:42:53  jaatroko
   outbound unix sockets (default)

   Revision 1.21  2002/12/01 16:06:11  ttakanen
   Fixed tty delay settings in cfg

   Revision 1.20  2002/11/25 13:44:59  tlilja
   Modified configuration file socket syntax

   Revision 1.19  2002/11/22 20:20:13  tlilja
   *** empty log message ***

   Revision 1.18  2002/11/21 18:53:48  tlilja
   Rewrote socket name parsing to use flex and yacc.

   Revision 1.17  2002/11/21 15:41:38  tlilja
   Added support for 'irq' and 'vendor' configuration parameters.

   Revision 1.16  2002/11/20 23:15:35  ttakanen
   *** empty log message ***

   Revision 1.15  2002/11/20 12:44:26  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.14  2002/11/19 19:15:49  tlilja
   Updated boot parameters and config system

   Revision 1.13  2002/11/11 15:59:27  tlilja
   *** empty log message ***

   Revision 1.12  2002/11/11 15:53:50  tlilja
   *** empty log message ***

   Revision 1.11  2002/11/11 15:28:27  tlilja
   Updated copyright notices

   Revision 1.10  2002/11/11 12:56:56  lsalmela
   fixed mtu in nic_init

   Revision 1.9  2002/11/06 14:45:07  tlilja
   Device creation now checks return values

   Revision 1.8  2002/11/06 14:12:57  tlilja
   *** empty log message ***

   Revision 1.7  2002/11/06 12:52:48  ttakanen
   Harmonized IO device headers

   Revision 1.6  2002/11/05 21:09:15  javirta2
   Shutdown device.

   Revision 1.5  2002/11/04 21:29:32  javirta2
   Added RTC-device. Compile with 'ACMEDEVICES' defined.

   Revision 1.4  2002/10/31 15:19:14  tlilja
   Added mtu parameter

   Revision 1.3  2002/10/31 11:57:23  ttakanen
   Partially working version of tty

   Revision 1.2  2002/10/10 17:02:23  tlilja
   *** empty log message ***

   Revision 1.1  2002/10/10 16:53:44  tlilja
   Configuration file support added.

*/

#include <stdarg.h>
#include <string.h>

#include "includes.h"

#include "cfg.h"
#include "cfg-parser.h"
#include "simulator.h"

#include "io.h"

#include "tty.h"
#include "nic.h"
#include "disk.h"
#include "plugio.h"

#include "device.h"

int cfg_parse(void);

static uint32_t clock_speed = 0;
static uint32_t mem_pages = 0;
static uint32_t num_cpus = 0;
static uint32_t cpu_irq = CPU_IRQ_DEFAULT;

int cfg_socketdomain(int type)
{
  if (type == CFG_SOCKET_UNIX)
    return AF_UNIX;
  else if (type == CFG_SOCKET_NET)
    return PF_INET;
  else {
    printf("yams: invalid socket type\n");
    exit(1);
  }

}

char *cfg_checksocketname(char *name)
{
  if (strlen(name) > 0)
    return name;
  printf("invalid socket name: %s", name);
  exit(1);
}
/*
 * Simulator options handling
 */
int cfg_simdefined()
{
  return clock_speed > 0 && mem_pages > 0 && num_cpus > 0;
}

int cfg_simoptions_start(void)
{
  if (cfg_simdefined()) {
    fprintf(stderr, "Simulator configuration already defined.");
    exit(1);
  }

  return 1;
}

int cfg_simoption_clockspeed(uint32_t speed)
{
  clock_speed = speed;
  return 1;
}

int cfg_simoption_memory(uint32_t memory)
{
  mem_pages = memory;
  return 1;
}

int cfg_simoption_cpus(uint32_t cpus)
{
  num_cpus = cpus;
  return 1;
}

int cfg_simoption_cpuirq(uint32_t irq)
{
  if (irq < 0 || irq > 5) {
    printf("\
yams: error in configuration file. Invalid IRQ line for inter-CPU \
    interrupts.");
    exit(1);
  }
  cpu_irq = irq;
  return 1;
}

int cfg_simoption_endianness(int bigendian) {
  simulator_bigendian = bigendian;
  return 1;
}

int cfg_simoptions_end(void)
{
  if (!cfg_simdefined()) {
    printf("\
yams: error in configuration file. All necessary options for\n\
    section \"Simulator\" were NOT given.\n");
    exit(1);
  }

  return 1;
}

cfg_device_t cfg_devices[CFG_MAX_DEVICES];
cfg_device_t *cfg_dev_ptr;
int cfg_maxdev = -1;

int cfg_deviceoptions_init(void)
{
  if (cfg_maxdev > CFG_MAX_DEVICES) {
    printf("\
yams: error can't define more than %d devices.\n", CFG_MAX_DEVICES);
    exit(1);
  }
  cfg_maxdev++;
  cfg_dev_ptr = &(cfg_devices[cfg_maxdev]);
  return 1;
}

int cfg_diskoptions_check(void)
{
  /* XXX Should do sanity checking of user given arguments */
  return 1;
}

int cfg_ttyoptions_check(void)
{
  /* XXX Should do sanity checking of user given arguments */
  return 1;
}

int cfg_nicoptions_check(void)
{
  /* XXX Should do sanity checking of user given arguments */
  return 1;
}

int cfg_plugoptions_check(void)
{
  /* XXX Should do sanity checking of user given arguments */
  return 1;
}


/*
 *  Configuration file reading and setting up the system.
 */
extern FILE *cfg_in;

int cfg_read(char *cfg_file)
{
  FILE *fp;
  int i;

  if ((fp = fopen(cfg_file, "rb")) == NULL)
    return 0;

  /* zero the devices */
  for (i=0; i<CFG_MAX_DEVICES; i++) {
    memset(&cfg_devices[i], 0, sizeof(cfg_device_t));
    cfg_devices[i].irq = -1; /* no irq by default */
  }

  printf("Reading configuration from '%s'\n", cfg_file);
  cfg_in = fp; /* set up the lexer */

  cfg_parse();
  fclose(fp);

  return 1;
}


int cfg_read_etc(void)
{
  /* XXX /etc Should be autoconf configurable */
  return cfg_read("/etc/yams.conf");
}

int cfg_get_home_path(char *dest)
{
  char *ptr;

  if ((ptr = getenv("HOME")) == NULL)
    return 0;

  strncpy(dest, ptr, MAX_FILENAME_LENGTH);

  return 1;
}

int cfg_read_home(void)
{
  char filename[MAX_FILENAME_LENGTH];

  if (cfg_get_home_path(filename)) {
    strncat(filename, "/.yams.conf", MAX_FILENAME_LENGTH);
    return cfg_read(filename);
  } else
    return 0;
}

int cfg_read_cwd(void)
{
  char filename[MAX_FILENAME_LENGTH];

  getcwd(filename, MAX_FILENAME_LENGTH);
  strncat(filename, "/yams.conf", MAX_FILENAME_LENGTH);

  return cfg_read(filename);
}

int cfg_init(void)
{
  int i;

  if (cfg_simdefined()) {
    simulator_create(mem_pages, num_cpus, clock_speed * 1000, cpu_irq);
  } else {
    printf(
"yams: simulator not created. (Configuration file didn't specify all\n\
    necessary options.\n");
    exit(1);
  }

  for (i = 0; i <= cfg_maxdev; i++) {
    device_t *dev = NULL;
    int retval;

    switch (cfg_devices[i].type) {
    case CFG_DISK:
      dev = disk_create();
      if ((retval = disk_init(cfg_devices[i].filename,
              cfg_devices[i].sectorsize,
              cfg_devices[i].numsectors,
              dev))) {
        printf("yams: Unable to create disk device (code: %d).\n",
             retval);
        exit(1);
      }
      if ((retval = disk_setparams(cfg_devices[i].numcylinders,
                    cfg_devices[i].time_rot,
                    cfg_devices[i].time_fullseek,
                    dev))) {
        printf("yams: Unable to create disk device (code: %d).\n",
             retval);
        exit(1);
      }
      device_set_irq(dev, cfg_devices[i].irq);
      device_set_vendor(dev, cfg_devices[i].vendor);
      break;
    case CFG_TTY:
      dev = tty_create();
      if ((retval = tty_init(cfg_devices[i].domain, cfg_devices[i].name,
                   cfg_devices[i].port, cfg_devices[i].listen,
                   dev))) {
        printf("yams: Unable to create tty device (code: %d).\n",
             retval);
        exit(1);
      }
      device_set_irq(dev, cfg_devices[i].irq);
      device_set_vendor(dev, cfg_devices[i].vendor);
      tty_setdelay(cfg_devices[i].send_delay, dev);
      break;
    case CFG_NIC:
      dev = nic_create();
      if ((retval = nic_init(cfg_devices[i].domain,
                   cfg_devices[i].name,
                   cfg_devices[i].port,
                   cfg_devices[i].mtu,
                   dev))) {
        printf("yams: Unable to create nic device (code: %d).\n",
             retval);
        exit(1);
      }

      nic_setreliability(cfg_devices[i].reliability, dev);
      nic_sethwaddr(cfg_devices[i].hwaddr, dev);
      nic_setdelays(cfg_devices[i].dma_delay, cfg_devices[i].send_delay,
             dev);
      device_set_irq(dev, cfg_devices[i].irq);
      device_set_vendor(dev, cfg_devices[i].vendor);
      break;
    case CFG_PLUGIN:
      if ((retval = plugio_init(cfg_devices[i].domain,
                    cfg_devices[i].name,
                    cfg_devices[i].port,
                    cfg_devices[i].listen,
                    cfg_devices[i].irq,
                    cfg_devices[i].async,
                    cfg_devices[i].options))) {
        printf("yams: unable to initialize pluggable I/O device "
             "(code: %d).\n", retval);
        exit(1);
      }
      /* don't add a device, plugio_init() does that: */
      dev = NULL;
      break;
    default:
      /* Never reached */
      break;
    }
    if (dev != NULL)
      simulator_add_device(dev);

  }

  return 1;
}
