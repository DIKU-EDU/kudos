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

   $Id: tty.c,v 1.29 2005/04/14 17:49:01 jaatroko Exp $
*/
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <tty.h>

#include "misc.h"
#include "device.h"
#include "simulator.h"
#include "async_input.h"

device_t *tty_create() {
    device_t *dev;
    ttydevice_t *tdev;

    dev = (device_t *) smalloc(sizeof(device_t));
    tdev = (ttydevice_t *) smalloc(sizeof(ttydevice_t));
    memset(tdev, 0, sizeof(ttydevice_t));

    dev->realdevice = tdev;

    dev->typecode = TYPECODE_TTY;
    memcpy(dev->vendor_string, "TTY-FAKE", 8);
    dev->irq = IRQ_TTY;
    dev->io_length = IOLENGTH_TTY;

    dev->io_write = &tty_io_write;
    dev->io_read  = &tty_io_read;
    dev->update   = &tty_update;

    tdev->irq_processor = 0;
    tdev->rirq_pending   = 0;
    tdev->wirq_pending   = 0;
    tdev->inbuf  = -1;
    tdev->outbuf = -1;

    return dev;
}

void tty_destroy(device_t *tty_dev) {
    free(tty_dev->realdevice);
    free(tty_dev);
}

static int tty_init_inet(char *name, int port, int slisten, ttydevice_t *tty) {
    struct sockaddr_in sin;
    struct sockaddr_in client_addr;
    socklen_t length;
    char buf[128];
    int opt_true = 1;
    struct hostent *hp;

    if(setsockopt(tty->sock, SOL_SOCKET, 
		  SO_REUSEADDR, &opt_true, sizeof(opt_true)) < 0) {
	perror("setsockopt failed for tty socket");
	return 1;
    }

    sin.sin_family = AF_INET;

    sin.sin_port = htons(port);
    hp  = gethostbyname(name);
    if(hp != NULL && hp->h_addr_list != NULL) {
	memcpy(&sin.sin_addr.s_addr, *hp->h_addr_list, 
	       sizeof(sin.sin_addr.s_addr));
    } else {
	if (slisten && name[0] == '\0') {
	    /* empty host name, listen to all local addresses */
	    sin.sin_addr.s_addr = INADDR_ANY;
	} else {
	    perror("Name resolution failed for TTY server host");
	    return 1;
	}
    }

    if(slisten) {
	/* we wait for a connection */

	if(bind(tty->sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	    perror("Bind to tty socket failed");
	    return 1;
	}

	if (sin.sin_addr.s_addr != INADDR_ANY)
	    printf("Waiting for TTY connection at %s:%d.\n",
		   name,
		   tty->sock_port);
	else
	    printf("Waiting for TTY connection at TCP-port %d.\n",
		   tty->sock_port);

	if(listen(tty->sock, 1) < 0) {
	    perror("Listen on tty socket failed");
	    return 1;
	}

	length=sizeof(client_addr);
	tty->desc = accept(tty->sock, 
			   (struct sockaddr *) &client_addr, 
			   &length);

	if(tty->desc < 0) {
	    perror("Accept failed for tty socket");
	    return 1;
	}

	inet_ntop(client_addr.sin_family, 
		  &client_addr.sin_addr, 
		  buf, 
		  sizeof(buf));
	printf("TTY connection from %s port %d\n", buf, client_addr.sin_port);
    } else {
	/* we connect */

	printf("Connecting to TTY at %s:%d\n", name, port);
	
	if(connect(tty->sock, (struct sockaddr *) &sin, sizeof(sin)) 
	   < 0) {
	    perror("Connection to TTY failed");
	    return 1;
	}

	tty->desc = tty->sock;

	printf("TTY connection established\n");
    }

    return 0;
}

static int tty_init_unix(char *name, int port, int slisten, ttydevice_t *tty) {
    struct sockaddr_un suna;

    if (slisten) {
	/* we wait for a connection */

	/* remove old socket file */
	unlink(name);

	memset(&suna, 0, sizeof(suna));
	suna.sun_family = AF_UNIX;
	strncpy(suna.sun_path, name, sizeof(suna.sun_path));

	if(bind(tty->sock, (struct sockaddr *) &suna, sizeof(suna)) < 0) {
	    perror("Bind to TTY Unix domain socket failed");
	    return 1;
	}

	printf("Waiting for TTY connection at Unix Domain Socket '%s'.\n", 
	       name);

	if(listen(tty->sock, 1) < 0) {
	    perror("Listen on TTY socket failed");
	    return 1;
	}

	tty->desc = accept(tty->sock, NULL, 0);

	if(tty->desc < 0) {
	    perror("Accept failed for TTY socket");
	    return 1;
	}
    } else {
	/* we connect */

	printf("Connecting to TTY at Unix Domain Socket '%s'\n", name);
	
	memset(&suna, 0, sizeof(suna));
	suna.sun_family = AF_UNIX;
	strncpy(suna.sun_path, name, sizeof(suna.sun_path));
	
	if (connect(tty->sock, (struct sockaddr *)&suna, sizeof(suna)) < 0) {
	    perror("Connection to TTY failed");
	    return 1;
	}

	tty->desc = tty->sock;
    }

    printf("TTY connection established\n");

    return 0;
}


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

#define WELCOME_STRING_MAX 256

int tty_init(int domain, char *name, int port, int listen, device_t *tty_dev) {
    int ret=0;
    ttydevice_t *tty;
    char welcome[WELCOME_STRING_MAX+1];

    tty = (ttydevice_t*)tty_dev->realdevice;

    tty->sock_port = port;
    tty->sock_domain = domain;
    tty->sock_name   = (name == NULL ? NULL : strdup(name));

    tty->wirq_pending = 0;
    tty->wirq_enabled = 1;
    tty->rirq_pending = 0;


    tty->sock = socket(domain, SOCK_STREAM, 0);
    if(tty->sock < 0) {
	perror("Can't open socket for tty");
	exit(1);
    }

    if(domain == PF_INET) {
	ret = tty_init_inet(name, port, listen, tty);
    } else if(domain == PF_UNIX) {
	ret = tty_init_unix(name, port, listen, tty);
    } else {
	printf("Invalid protocol domain %d for tty socket\n", domain);
	return 2;
    }

    async_input_register_fd(tty->desc);

    if(ret == 0) {
	snprintf(welcome, WELCOME_STRING_MAX,
		 "Welcome. This is YAMS virtual terminal.\n\n");

	write(tty->desc, welcome, strlen(welcome));
    }

    return ret;
}

/*returns: 0 - OK, 1 - invalid parameters */

int tty_setdelay(int delay, device_t *tty_dev) {
    ttydevice_t *tty;
    tty = (ttydevice_t*)tty_dev->realdevice;

    if (tty == NULL) return -1;
    if (delay < 0) return 1;
    tty->send_delay = delay;
    return 0;
}

int tty_io_write(device_t *dev, uint32_t addr, uint32_t data) {
    ttydevice_t *tty = dev->realdevice;
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */

    switch(addr) {
    case TTY_PORT_STATUS:
	/* this port is read-only, we will ignore writes */
	break;

    case TTY_PORT_COMMAND:
	tty->error_status = TTY_ERROR_ICOMM;

	if(data == TTY_COMMAND_RESET_READ_IRQ) {
	    tty->error_status = 0;
	    tty->rirq_pending = 0;
	}

	if(data == TTY_COMMAND_RESET_WRITE_IRQ) {
	    tty->error_status = 0;
	    tty->wirq_pending = 0;
	}

	if(data == TTY_COMMAND_ENABLE_WIRQ) {
	    tty->error_status = 0;
	    tty->wirq_enabled = 1;
	}

	if(data == TTY_COMMAND_DISABLE_WIRQ) {
	    tty->error_status = 0;
	    tty->wirq_enabled = 0;
	}

	break;

    case TTY_PORT_DATA:
	{
	    uint8_t byte;

	    if(tty->outbuf < 0) {
		byte = data;
		tty->outbuf = (int) byte;
	    } else {
		/* buffer full, ignore data */
	    }
	}
	break;
	    
    default:
	printf("Warning: mysterious IO-write in tty device. (Internal error)\n");
	break;
    }
    
    return 0;
}

int tty_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
    ttydevice_t *tty = dev->realdevice;
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */

    switch(addr) {
    case TTY_PORT_STATUS:
	{
	    uint32_t status = tty->error_status;

	    if(tty->inbuf >= 0) {
		status += 1 << TTY_STATUS_RAVAIL;
	    }

	    if(tty->outbuf >=0) {
		status += 1 << TTY_STATUS_WBUSY;
	    }

	    if(tty->rirq_pending) {
		status += 1 << TTY_STATUS_RIRQ;
	    }

	    if(tty->wirq_pending) {
		status += 1 << TTY_STATUS_WIRQ;
	    }

	    if(tty->wirq_enabled) {
		status += 1 << TTY_STATUS_WIRQ_ENABLED;
	    }

	    *data = status;
	}
	break;

    case TTY_PORT_COMMAND:
	/* ignore reads from command ports */
	*data = 0;
	break;

    case TTY_PORT_DATA:
	if(tty->inbuf >= 0) {
	    *data = ((uint32_t) tty->inbuf) & 0x000000ff;
	    tty->inbuf = -1;
	} else {
	    /* no data */
	    *data = 0;
	}

	break;
	    
    default:
	printf("Warning: mysterious IO-read in tty device. (Internal error)\n");
	break;
    }

    return 0;
}

static void tty_raise_read_interrupt(ttydevice_t *tty) {
    if(!tty->wirq_pending && !tty->rirq_pending) {
	tty->irq_processor = select_cpu_for_irq();
    }

    tty->rirq_pending = 1;
}

static void tty_raise_write_interrupt(ttydevice_t *tty) {
    if(!tty->wirq_pending && !tty->rirq_pending) {
	tty->irq_processor = select_cpu_for_irq();
    }

    tty->wirq_pending = 1;
}

int tty_update(device_t *dev) {
    int delay;
    ttydevice_t *tty;
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    tty = dev->realdevice;
    
    delay = (tty->send_delay) * hardware->clockspeed / 1000 + 1;

    if(((int)(hardware->cycle_count & 0x0ffffff)) % delay == 0) {
	/* write pending writes */
	if(tty->outbuf >= 0) {
	    int ret;
	    uint8_t byte;

	    byte = (uint8_t) tty->outbuf;
	    ret  = write(tty->desc, &byte, 1);

	    if(ret == 1) {
		tty->outbuf = -1;
		tty_raise_write_interrupt(tty);
	    }
	}
    }


    if(tty->inbuf < 0) {
	/* check reading socket for available data */
	if (async_input_check_fd(tty->desc)) {
	    uint8_t byte;
	    int ret;

	    /* read socket */	    
	    ret = read(tty->desc, &byte, 1);
	    
	    if(ret > 0) {
		/* byte read successfully */
		tty->inbuf = byte;
		tty_raise_read_interrupt(tty);
	    } else if(ret == 0) {
		/* file end, ignore */
	    } else {
		/* read error */
		if(errno == EINTR) {
		    /* signal interrupted read, no problem, 
		       we'll poll later */
		} else {
		    /* some other error than signal */
		    perror("Read from console stream failed");
		}
	    }
	}
    }

    if(tty->rirq_pending || (tty->wirq_pending && tty->wirq_enabled)) {
	RAISE_HW_INTERRUPT(tty->irq_processor, dev->irq);
    }
    
    return 0;
}
