/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: tty.h,v 1.9 2003/05/13 15:42:54 jaatroko Exp $
*/

#include "device.h"

#ifndef YAMS_TTY_H
#define YAMS_TTY_H

#define TTY_PORT_STATUS 0x00
#define TTY_PORT_COMMAND 0x04
#define TTY_PORT_DATA 0x08

#define TTY_COMMAND_RESET_READ_IRQ 0x01
#define TTY_COMMAND_RESET_WRITE_IRQ 0x02
#define TTY_COMMAND_ENABLE_WIRQ 0x03
#define TTY_COMMAND_DISABLE_WIRQ 0x04

#define TTY_STATUS_RAVAIL 0
#define TTY_STATUS_WBUSY 1
#define TTY_STATUS_RIRQ 2
#define TTY_STATUS_WIRQ 3
#define TTY_STATUS_WIRQ_ENABLED 4
#define TTY_ERROR_ICOMM 0x20000000

#define IRQ_TTY 2
#define IOLENGTH_TTY 12

typedef struct {
    int send_delay;
    int sock  ;
    int desc;
    char *sock_name;
    int sock_port;
    int sock_domain;

    int inbuf; /* -1 == empty, othervice contains number [0,255] (byte) */
    int outbuf; /* -1 == empty, ohtervice contains number [0,255] (byte) */
    int irq_processor;

    int wirq_pending;
    int wirq_enabled;
    int rirq_pending;

    uint32_t error_status;
} ttydevice_t;

/* Allocates memory for a disk device */
device_t *tty_create();
void tty_destroy(device_t *tty);

/* Creates a new TTY device
 * domain: either PF_INET or PF_UNIX
 * name: PF_UNIX: name of the socket file
 *       PF_INET: name of the remote host
 * port: port to connect to/listen. Ignored for PF_UNIX
 * listen: whether to connect or listen to the socket
 *
 * returns:
 * 0 - OK
 * 1 - bind() failed for the socket
 * 2 - invalid parameters
 */
int tty_init(int domain, char *name, int port, int listen,
	     device_t *tty);

/*returns: 0 - OK, 1 - invalid parameters */
int tty_setdelay(int delay, device_t *tty);

int tty_io_write(device_t *dev, uint32_t addr, uint32_t data);
int tty_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int tty_update(device_t *dev);



#endif /* YAMS_TTY_H */
