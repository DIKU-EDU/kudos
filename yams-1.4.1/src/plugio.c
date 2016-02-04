/* yams -- Yet Another Machine Simulator
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

   $Id: plugio.c,v 1.3 2010/11/24 16:52:19 jaatroko Exp $
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

#include "includes.h"
#include "misc.h"
#include "plugio.h"
#include "device.h"
#include "simulator.h"
#include "async_input.h"

static pluggable_device_t *plugio_devices = NULL;

/* Linear search for the device with tag and fd */
static pluggable_device_t *lookup_dev(int fd, int tag) {
    pluggable_device_t *plugio;

    for (plugio = plugio_devices; plugio != NULL; plugio = plugio->next)
	if (plugio->fd == fd && plugio->tag == tag)
	    return plugio;
    return NULL;
}

static device_t *plugio_create() {
    device_t *dev;
    pluggable_device_t *plugio;

    dev = (device_t *) smalloc(sizeof(device_t));
    plugio = (pluggable_device_t *) smalloc(sizeof(pluggable_device_t));
    memset(plugio, 0, sizeof(pluggable_device_t));

    dev->realdevice = plugio;

    dev->typecode = 0;
    memcpy(dev->vendor_string, "????????", 8);
    dev->irq = -1;
    dev->io_length = 0;

    dev->io_write = &plugio_io_write;
    dev->io_read  = &plugio_io_read;
    dev->update   = &plugio_update;

    plugio->irq_processor = 0;
    plugio->irq_pending = -1;

    return dev;
}

void plugio_destroy(device_t *plugio_dev) {
    free(plugio_dev->realdevice);
    free(plugio_dev);
}


static int plugio_init_inet(char *name, int port, int slisten,
			    int sock, int *desc)
{
    struct sockaddr_in sin;
    struct sockaddr_in client_addr;
    socklen_t length;
    char buf[128];
    int opt_true = 1;
    struct hostent *hp;

    if(setsockopt(sock, SOL_SOCKET, 
		  SO_REUSEADDR, &opt_true, sizeof(opt_true)) < 0) {
	perror("setsockopt failed for PLUGIO socket");
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
	    perror("Name resolution failed for PLUGIO server host");
	    return 1;
	}
    }

    if(slisten) {
	/* we wait for a connection */

	if(bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	    perror("Bind to PLUGIO socket failed");
	    return 1;
	}

	if (sin.sin_addr.s_addr != INADDR_ANY)
	    printf("Waiting for PLUGIO connection at %s:%d.\n",
		   name,
		   port);
	else
	    printf("Waiting for PLUGIO connection at TCP-port %d.\n",
		   port);

	if(listen(sock, 1) < 0) {
	    perror("Listen on PLUGIO socket failed");
	    return 1;
	}

	length=sizeof(client_addr);
	*desc = accept(sock, 
		       (struct sockaddr *) &client_addr, 
		       &length);

	if(*desc < 0) {
	    perror("Accept failed for PLUGIO socket");
	    return 1;
	}

	inet_ntop(client_addr.sin_family, 
		  &client_addr.sin_addr, 
		  buf, 
		  sizeof(buf));
	printf("PLUGIO connection from %s port %d\n",
	       buf, client_addr.sin_port);
    } else {
	/* we connect */

	printf("Connecting to PLUGIO at %s:%d\n", name, port);
	
	if(connect(sock, (struct sockaddr *) &sin, sizeof(sin)) 
	   < 0) {
	    perror("Connection to PLUGIO failed");
	    return 1;
	}

	*desc = sock;

	printf("PLUGIO connection established\n");
    }

    return 0;
}

static int plugio_init_unix(char *name, int port, int slisten,
			    int sock, int *desc)
{
    struct sockaddr_un suna;

    if (slisten) {
	/* we wait for a connection */

	/* remove old socket file */
	unlink(name);

	memset(&suna, 0, sizeof(suna));
	suna.sun_family = AF_UNIX;
	strncpy(suna.sun_path, name, sizeof(suna.sun_path));

	if(bind(sock, (struct sockaddr *) &suna, sizeof(suna)) < 0) {
	    perror("Bind to PLUGIO Unix domain socket failed");
	    return 1;
	}

	printf("Waiting for PLUGIO connection at Unix Domain Socket '%s'.\n", 
	       name);

	if(listen(sock, 1) < 0) {
	    perror("Listen on PLUGIO socket failed");
	    return 1;
	}

	*desc = accept(sock, NULL, 0);

	if(*desc < 0) {
	    perror("Accept failed for PLUGIO socket");
	    return 1;
	}
    } else {
	/* we connect */

	printf("Connecting to PLUGIO at Unix Domain Socket '%s'\n", name);
	
	memset(&suna, 0, sizeof(suna));
	suna.sun_family = AF_UNIX;
	strncpy(suna.sun_path, name, sizeof(suna.sun_path));
	
	if (connect(sock, (struct sockaddr *)&suna, sizeof(suna)) < 0) {
	    perror("Connection to PLUGIO failed");
	    return 1;
	}

	*desc = sock;
    }

    printf("PLUGIO connection established\n");

    return 0;
}



typedef struct {
    uint32_t cmd;
    uint32_t type;
    uint32_t nports;
    uint32_t irq;
    char vendor[8];
    uint32_t mmap;
} device_cmd_t;


static void connection_lost() {
    printf("PLUGIO: protocol error or connection lost, shutting down.\n");
    exit(1);
}

/* Wrappers for read(2) and write(2) that either read/write all of the
 * data or exit with an error.
 */
static void READ(int fd, void* rbuf, size_t count) {
    ssize_t r;
    char *buf = rbuf;
    
    while ((r = read(fd, buf, count)) != count) {
	if (r == 0)
	    connection_lost();
	if (r < 0 && errno != EINTR) {
	    printf("PLUGIO: socket read error: %s\n", strerror(errno));
	    connection_lost();
	}
	if (r < 0) r = 0; /* try again on EINTR */
	count -= r;
	buf += r;
    }
}

static void write_wrap(int fd, void* rbuf, size_t count) {
    ssize_t r;
    char *buf = rbuf;
    
    while ((r = write(fd, buf, count)) != count) {
	if (r == 0)
	    connection_lost();
	if (r < 0 && errno != EINTR) {
	    printf("PLUGIO: socket write error: %s\n", strerror(errno));
	    connection_lost();
	}
	if (r < 0) r = 0; /* try again on EINTR */
	count -= r;
	buf += r;
    }
}

/* Buffered version, buffer flushed when 'now' is nonzero. This may
 * reduce socket traffic as commands and arguments are written as one
 * chunk.
 */
#define WRITE_BUFSIZE 32
static void WRITE(int fd, void* rbuf, size_t count, int now) {
    static char buffer[WRITE_BUFSIZE];
    static int fill = 0;

    /* Flush buffer if data does not or will not fit. */
    if (fill + count > WRITE_BUFSIZE || count > WRITE_BUFSIZE) {
	if (fill > 0) write_wrap(fd, buffer, fill);
	fill = 0;
    }

    /* If it still won't fit, write directly from rbuf */
    if (count > WRITE_BUFSIZE) {
	write_wrap(fd, rbuf, count);
	return;
    }

    /* Copy into buffer */
    memcpy(buffer + fill, rbuf, count);
    fill += count;

    /* Write if requested */
    if (now) {
	write_wrap(fd, buffer, fill);
	fill = 0;
    }
}


int plugio_init(int domain, char *name, int port, int listen,
		int irq, int async, char *options)
{
    int sock, desc = -1;
    device_cmd_t cmd;
    uint32_t data;
    int ret;
    device_t *dev;
    pluggable_device_t *plugio;

    sock = socket(domain, SOCK_STREAM, 0);
    if(sock < 0) {
	perror("Can't open socket for PLUGIO");
	exit(1);
    }

    if(domain == PF_INET) {
	ret = plugio_init_inet(name, port, listen, sock, &desc);
    } else if(domain == PF_UNIX) {
	ret = plugio_init_unix(name, port, listen, sock, &desc);
    } else {
	printf("Invalid protocol domain %d for PLUGIO socket\n", domain);
	return 2;
    }

    if (ret != 0)
	return ret;

    if (async)
	async_input_register_fd(desc);

    if (options == NULL)
	options = "";

    /* Send INIT command */
    data = htonl(PLUGIO_MAKECMD((async ? PLUGIO_FLAG_ASYNC : 0)
				| (simulator_bigendian ? 0 : PLUGIO_FLAG_WORDSLE),
				0,
				PLUGIO_CMD_INIT, 0));
    WRITE(desc, &data, 4, 0);
    data = htonl(hardware->num_cpus);
    WRITE(desc, &data, 4, 0);
    data = htonl(hardware->memory->pagesize * hardware->memory->num_pages);
    WRITE(desc, &data, 4, 0);
    data = htonl(irq);
    WRITE(desc, &data, 4, 0);
    data = htonl(strlen(options));
    WRITE(desc, &data, 4, data == 0);
    if (data != 0)
	WRITE(desc, options, strlen(options), 1);

    do {
	/* Read DEVICE replies */
	READ(desc, &cmd, sizeof(cmd));
	cmd.cmd = ntohl(cmd.cmd);
	cmd.type = ntohl(cmd.type);
	cmd.nports = ntohl(cmd.nports);
	cmd.irq = ntohl(cmd.irq);
	cmd.mmap = ntohl(cmd.mmap);

	if (PLUGIO_CMD(cmd.cmd) != PLUGIO_REPLY_DEVICE)
	    connection_lost();

	dev = plugio_create();
	plugio = (pluggable_device_t*)dev->realdevice;

	dev->typecode = cmd.type;
	memcpy(dev->vendor_string, cmd.vendor, 8);
	dev->irq = cmd.irq;
	dev->io_length = 4 * cmd.nports;

	plugio->next = NULL;
	plugio->tag = PLUGIO_TAG(cmd.cmd);
	plugio->async = (async && (cmd.cmd & PLUGIO_FLAG_ASYNC));
	plugio->fd = desc;
	plugio->mmap_size = cmd.mmap;
	plugio->mmap_base = 0;

	plugio->delayed_effect = UINT64_C(0xffffffffffffffff);
	plugio->timer = UINT64_C(0xffffffffffffffff);

	simulator_add_device(dev);
	plugio->next = plugio_devices;
	plugio_devices = plugio;
	if (cmd.mmap != 0)
	    simulator_add_mmap(plugio);
    } while((cmd.cmd & PLUGIO_FLAG_LAST) == 0);

    /* (MMAP commands are sent later, when the areas have been allocated) */

    return 0;
}

/* The requested MMAP area will be at addr */
int plugio_mmap(pluggable_device_t *plugio, uint32_t addr) {
    uint32_t cmd;

    if (plugio == NULL) return -1;

    plugio->mmap_base = addr;

    /* Send MMAP command */
    cmd = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_MMAP, 0));
    WRITE(plugio->fd, &cmd, 4, 0);
    addr = htonl(addr);
    WRITE(plugio->fd, &addr, 4, 1);

    return 0;
}



static void handle_replies(pluggable_device_t *pdev,
			   uint32_t *word, int async) {
    uint32_t cmd_word, data, addr, d;
    int last = 0, cmd;
    pluggable_device_t *plugio;
    char *buf;

    do {
	plugio = pdev; /* reset to this device (in case of async replies) */
	READ(plugio->fd, &d, 4);
	cmd_word = ntohl(d);
	last = (cmd_word & PLUGIO_FLAG_LAST);
	/* If we get async replies when expecting synchronous ones we
	 * do not stop with the LAST flag: */
	if (!async && (cmd_word & PLUGIO_FLAG_ASYNC))
	    last = 0;
	cmd = PLUGIO_CMD(cmd_word);
	/* lookup the right device for async replies: */
	if (cmd_word & PLUGIO_FLAG_ASYNC)
	    plugio = lookup_dev(plugio->fd, PLUGIO_TAG(cmd_word));

	/* Replys to different devices is a protocol error */
	if (plugio->tag != PLUGIO_TAG(cmd_word))
	    connection_lost();

	switch (cmd) {
	case PLUGIO_REPLY_OK:
	    break;
	case PLUGIO_REPLY_WORD:
	    READ(plugio->fd, &d, 4);
	    *word = ntohl(d);
	    break;
	case PLUGIO_REPLY_DATA:
	    READ(plugio->fd, &d, 4);
	    data = ntohl(d);
	    READ(plugio->fd, word, data);
	    break;
	case PLUGIO_REPLY_DELAY:
	    READ(plugio->fd, &d, 4);
	    data = ntohl(d);
	    if (data == 0) {
		READ(plugio->fd, &d, 4);
		data = ntohl(d);
		plugio->delayed_effect = hardware->cycle_count + 
		    (uint64_t)data * (uint64_t)hardware->clockspeed 
		    / UINT64_C(1000);
	    } else {
		plugio->delayed_effect = 
		    hardware->cycle_count + (uint64_t)data;
		READ(plugio->fd, &d, 4); /* read but ignore */
	    }
	    break;
	case PLUGIO_REPLY_TIMER:
	    READ(plugio->fd, &d, 4);
	    data = ntohl(d);
	    if (data == 0) {
		READ(plugio->fd, &d, 4);
		data = ntohl(d);
		plugio->timer = hardware->cycle_count + 
		    (uint64_t)data * (uint64_t)hardware->clockspeed 
		    / UINT64_C(1000);
	    } else {
		plugio->timer = 
		    hardware->cycle_count + (uint64_t)data;
		READ(plugio->fd, &d, 4); /* read but ignore */
	    }
	    break;
	case PLUGIO_REPLY_IRQ:
	    READ(plugio->fd, &d, 4);

	    if (plugio->irq_pending >= 0)
		connection_lost();

	    plugio->irq_pending = ntohl(d);
	    if (plugio->irq_pending > 5)
		plugio->irq_pending = -1;

	    plugio->irq_processor = select_cpu_for_irq();

	    break;
	case PLUGIO_REPLY_CPUIRQ:
	    READ(plugio->fd, &d, 4);
	    plugio->irq_processor = ntohl(d);

	    if (plugio->irq_pending >= 0)
		connection_lost();

	    READ(plugio->fd, &d, 4);
	    plugio->irq_pending = ntohl(d);
	    if (plugio->irq_pending > 5)
		plugio->irq_pending = -1;

	    break;
	case PLUGIO_REPLY_CLIRQ:
	    plugio->irq_pending = -1;
	    
	    break;
	case PLUGIO_REPLY_DMAW:
	    READ(plugio->fd, &d, 4);
	    addr = ntohl(d);
	    READ(plugio->fd, &d, 4);
	    data = ntohl(d);

	    if (addr + data > 
		hardware->memory->pagesize * hardware->memory->num_pages) {
		printf("PLUGIO: memory write out of bounds.\n");
		connection_lost();
	    }

	    buf = (char*)hardware->memory->physmem + addr;
	    READ(plugio->fd, buf, data);

	    break;
	case PLUGIO_REPLY_DMAR:
	    READ(plugio->fd, &d, 4);
	    addr = ntohl(d);
	    READ(plugio->fd, &d, 4);
	    data = ntohl(d);

	    if (addr + data > 
		hardware->memory->pagesize * hardware->memory->num_pages) {
		printf("PLUGIO: memory read out of bounds.\n");
		connection_lost();
	    }

	    d = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_REPLY_DATA, 0));
	    WRITE(plugio->fd, &d, 4, 0);
	    d = htonl(data);
	    WRITE(plugio->fd, &d, 4, 0);
	    buf = (char*)hardware->memory->physmem + addr;
	    WRITE(plugio->fd, buf, data, 1);

	    break;
	default:
	    connection_lost();
	}
    } while (!last);
}


/* addr is relative to the start of the area */
int plugio_mmap_read(pluggable_device_t *plugio, uint32_t addr,
		     void *buf, int size) {
    uint32_t d;

    d = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_DATAR, 0));
    WRITE(plugio->fd, &d, 4, 0);
    d = htonl(addr);
    WRITE(plugio->fd, &d, 4, 0);
    d = htonl(size);
    WRITE(plugio->fd, &d, 4, 1);

    handle_replies(plugio, buf, 0);
    return 0;
}

/* addr is relative to the start of the area */
int plugio_mmap_write(pluggable_device_t *plugio, uint32_t addr,
		      void *buf, int size) {
    uint32_t d;

    d = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_DATAW, 0));
    WRITE(plugio->fd, &d, 4, 0);
    d = htonl(addr);
    WRITE(plugio->fd, &d, 4, 0);
    d = htonl(size);
    WRITE(plugio->fd, &d, 4, 0);

    WRITE(plugio->fd, buf, size, 1);

    handle_replies(plugio, NULL, 0);
    return 0;
}


int plugio_io_write(device_t *dev, uint32_t addr, uint32_t data) {
    uint32_t io_port = htonl(addr >> 2);
    pluggable_device_t *plugio;
    uint32_t cmd;
   
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    plugio = dev->realdevice;

    cmd = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_PORTW, 0));
    WRITE(plugio->fd, &cmd, 4, 0);
    WRITE(plugio->fd, &io_port, 4, 0);
    data = htonl(data);
    WRITE(plugio->fd, &data, 4, 1);

    handle_replies(plugio, NULL, 0);

    return 0;
}

int plugio_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
    uint32_t io_port = htonl(addr >> 2);
    pluggable_device_t *plugio;
    uint32_t cmd;
   
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    plugio = dev->realdevice;

    cmd = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_PORTR, 0));
    WRITE(plugio->fd, &cmd, 4, 0);
    WRITE(plugio->fd, &io_port, 4, 1);

    handle_replies(plugio, data, 0);

    return 0;
}

int plugio_update(device_t *dev) {
    pluggable_device_t *plugio;
   
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    plugio = dev->realdevice;

    /* First check if there is a delayed effect ready to be actuated: */
    if (plugio->delayed_effect == hardware->cycle_count) {
	uint32_t d;

	plugio->delayed_effect = UINT64_C(0xffffffffffffffff);

	d = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_ADELAY, 0));
	WRITE(plugio->fd, &d, 4, 1);
	handle_replies(plugio, NULL, 0);
    }

    /* Then check for timer expiration: */
    if (plugio->timer == hardware->cycle_count) {
	uint32_t d;

	plugio->timer = UINT64_C(0xffffffffffffffff);

	d = htonl(PLUGIO_MAKECMD(0, plugio->tag, PLUGIO_CMD_ALARM, 0));
	WRITE(plugio->fd, &d, 4, 1);
	handle_replies(plugio, NULL, 0);
    }

    /* Handle asynchronous input: */
    if (plugio->async 
	&& async_input_check_fd(plugio->fd)
	&& async_input_verify_fd(plugio->fd)) {
	handle_replies(plugio, NULL, 1);
    }

    /* Raise IRQ line if it's pending: */
    if (plugio->irq_pending >= 0) {
	RAISE_HW_INTERRUPT(plugio->irq_processor, plugio->irq_pending);
    }

    return 0;
}
