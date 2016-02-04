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

   $Id: cfg.h,v 1.12 2010/11/24 13:33:20 jaatroko Exp $

   $Log: cfg.h,v $
   Revision 1.12  2010/11/24 13:33:20  jaatroko
   replace little-endian command line option with a configuration file option

   Revision 1.11  2005/06/05 14:19:31  jaatroko
   support for plugin configuration

   Revision 1.10  2005/04/09 14:47:50  jaatroko
   Fixed some compiler warnings

   Revision 1.9  2003/05/13 15:42:53  jaatroko
   outbound unix sockets (default)

   Revision 1.8  2002/11/25 13:45:00  tlilja
   Modified configuration file socket syntax

   Revision 1.7  2002/11/22 20:35:10  tlilja
   Added support for 255 -s arguments.

   Revision 1.6  2002/11/22 20:20:13  tlilja
   *** empty log message ***

   Revision 1.5  2002/11/21 18:53:48  tlilja
   Rewrote socket name parsing to use flex and yacc.

   Revision 1.4  2002/11/20 12:44:27  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.3  2002/11/11 15:59:28  tlilja
   *** empty log message ***

   Revision 1.2  2002/11/11 15:28:27  tlilja
   Updated copyright notices

   Revision 1.1  2002/10/10 16:53:45  tlilja
   Configuration file support added.

*/
#ifndef CFG_H
#define CFG_H

#define CFG_MAX_DEVICES 128

#define CFG_SOCKET_UNIX 1
#define CFG_SOCKET_NET  2

typedef struct {
    enum {CFG_DISK, CFG_TTY, CFG_NIC, CFG_PLUGIN} type;
    char *vendor;
    char *name;
    char *filename;
    char *options;

    int async;
    int irq;
    int sectorsize;
    int numsectors;
    int numcylinders;
    int time_rot;
    int time_fullseek;
    int domain;  /* SOCKET_UNIX or SOCKET_NET */
    int port;
    int listen; /* listen=1, connect=0 */
    int hwaddr;
    int send_delay;
    int mtu;
    int reliability;
    int dma_delay;
} cfg_device_t;

extern cfg_device_t cfg_devices[CFG_MAX_DEVICES];
extern int cfg_maxdev;
extern cfg_device_t* cfg_dev_ptr;

int cfg_socketdomain(int type);

int cfg_simoptions_start(void);
int cfg_simoption_clockspeed(uint32_t speed);
int cfg_simoption_memory(uint32_t memory);
int cfg_simoption_cpus(uint32_t cpus);
int cfg_simoption_cpuirq(uint32_t irq);
int cfg_simoption_endianness(int bigendian);
int cfg_simoptions_end(void);

int cfg_deviceoptions_init(void);

int cfg_diskoptions_check(void);
int cfg_ttyoptions_check(void);
int cfg_nicoptions_check(void);
int cfg_plugoptions_check(void);

#define MAX_FILENAME_LENGTH 255
#define MAX_SCRIPTS 255

int cfg_read(char *cfg_file);
int cfg_read_etc(void);
int cfg_read_home(void);
int cfg_read_cwd(void);
int cfg_init(void);

char *cfg_checksocketname(char *name);

#endif /* CFG_H */
