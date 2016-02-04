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

   $Id: nic.c,v 1.25 2005/06/05 15:00:21 jaatroko Exp $
*/
#include "includes.h"
#include "io.h"
#include "nic.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <dirent.h>
#include <time.h>

#include <arpa/inet.h>
#include "misc.h"
#include "simulator.h"

device_t *nic_create() {
    device_t *dev;
    nicdevice_t *ndev;

    dev = (device_t *) smalloc(sizeof(device_t));
    ndev = (nicdevice_t *) smalloc(sizeof(nicdevice_t));
    memset(ndev, 0, sizeof(nicdevice_t));

    dev->realdevice = ndev;

    dev->typecode = TYPECODE_NIC;
    memcpy(dev->vendor_string, "NIC-FAKE", 8);
    dev->irq = IRQ_NIC;
    dev->io_length = IOLENGTH_NIC;

    dev->io_write = &nic_io_write;
    dev->io_read  = &nic_io_read;
    dev->update   = &nic_update;

    return dev;
}

void nic_destroy(device_t *nic_dev) {
    nicdevice_t * nic;

    nic = (nicdevice_t *)nic_dev->realdevice;
    close(nic->sock);
    if (nic->sock_domain == PF_UNIX) {
        unlink(nic->sock_name);
    }
    free(nic_dev->realdevice);
    free(nic_dev);
}


static int nic_init_inet(char *name, int port, nicdevice_t *nic) {
    struct sockaddr_in sin;
    struct ip_mreq mreq;

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(nic->sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("NIC: bind failed");
        return 1;
    }

    nic->multicast_addr.sin_family = AF_INET;
    nic->multicast_addr.sin_port = htons(port);
    nic->multicast_addr.sin_addr.s_addr = inet_addr(name);

    mreq.imr_multiaddr.s_addr=inet_addr(name);
    mreq.imr_interface.s_addr=htonl(INADDR_ANY);

    if (setsockopt(nic->sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,
                   &mreq,sizeof(mreq)) < 0) {
        perror("NIC: cannot listen to multicast address");
        return 1;
    }

    return 0;
}

static int nic_init_unix(char *name, int port, nicdevice_t *nic) {
    struct sockaddr_un suna;
    char * index;

    /* remove old socket file */
    unlink(name);

    suna.sun_family = AF_UNIX;
    strncpy(suna.sun_path, name, sizeof(suna.sun_path));

    if (bind(nic->sock, (struct sockaddr *) &suna, sizeof(suna)) < 0) {
        perror("NIC: Bind to NIC Unix domain socket failed");
        return 1;
    }

    /* Determine socket directory */
    index = rindex(name, '/');
    if (index == NULL) {
        nic->dir = smalloc(3);
        strncpy(nic->dir, "./", 3);
    } else {
        nic->dir = smalloc(index - name + 2);
        strncpy(nic->dir, name, index - name + 1);
        nic->dir[index-name+1] = '\0';
    }

    return 0;
}

/* Creates a new NIC device
 * domain: either PF_INET or PF_UNIX
 * name: PF_UNIX: name of the socket file
 *       PF_INET: IP address of the multicast group
 * port: multicast port to listen, ignored if PF_UNIX
 *
 * returns:
 * 0 - OK
 * 1 - bind() failed for the socket, or some other error
 * 2 - invalid parameters
 */
int nic_init(int domain, char *name, int port, int mtu, device_t *nic_dev) {
    nicdevice_t *nic;
    int opt_true = 1;
    int flags;

    /* init random generator */
    srand((int) clock());

    nic = (nicdevice_t *)nic_dev->realdevice;

    if (nic == NULL) return -1;
    if (mtu < 10) return 1;
    nic->mtu = mtu;

    nic->recv_buffer = smalloc(nic->mtu*sizeof(uint8_t));
    nic->send_buffer = smalloc(nic->mtu*sizeof(uint8_t));

    nic->send_event = UINT64_C(0xffffffffffffffff);
    nic->recv_event = UINT64_C(0xffffffffffffffff);
    nic->reliability = 100;
    nic->send_delay = 0;
    nic->dma_delay = 0;
    nic->hwaddr = 0xffffffff;
    nic->status = 0;
    nic->dma_addr = 0xffffffff;
    nic->cpuirq = 0;

    nic = (nicdevice_t*)nic_dev->realdevice;
    nic->sock_port = port;
    nic->sock_domain = domain;
    nic->sock_name = strdup(name);

    nic->sock = socket(domain, SOCK_DGRAM, 0);

    if (nic->sock < 0) {
        perror("NIC: opening socket failed");
        return 1;
    }

    /* Use non-blocking io */
    flags = fcntl(nic->sock, F_GETFL);
    flags |= O_NONBLOCK;
    if (fcntl(nic->sock, F_SETFL, flags) == -1) {
        perror("NIC: failed to make socket non-blocking");
        return 1;
    }

    if (setsockopt(nic->sock, SOL_SOCKET,
                   SO_REUSEADDR, &opt_true, sizeof(opt_true)) < 0) {
        perror("setsockopts failed for nic socket");
        return 1;
    }

    if(domain == PF_INET) {
        if (nic_init_inet(name, port, nic))
            return 1;
    } else if (domain == PF_UNIX) {
        if (nic_init_unix(name, port, nic))
            return 1;
    } else {
        printf("Invalid protocol domain %d for nic socket\n", domain);
        return 1;
    }

    return 0;
}

/* returns: 0 - OK, 1 - invalid parameters (e.g. rel not in 0..100) */
int nic_setreliability(int reliability, device_t *nic_dev) {
    nicdevice_t *nic;
    nic = (nicdevice_t*)nic_dev->realdevice;

    if (nic == NULL) return -1;
    if (reliability < 0 || reliability > 100) return 1;
    nic->reliability = reliability;
    return 0;
}

/* returns: 0 - OK, 1 - invalid parameters */
int nic_sethwaddr(uint32_t hwaddr, device_t *nic_dev) {
    nicdevice_t *nic;
    nic = (nicdevice_t*)nic_dev->realdevice;

    if (nic == NULL) return -1;
    /* reserved for "network" and broadcast */
    if (hwaddr == 0 || hwaddr == 0xffffffff) return 1;
    nic->hwaddr = htonl(hwaddr);
    return 0;
}

/* returns: 0 - OK, 1 - invalid parameters */
int nic_setdelays(int dma_delay, int send_delay, device_t *nic_dev) {
    nicdevice_t *nic;
    nic = (nicdevice_t*)nic_dev->realdevice;

    if (nic == NULL) return -1;
    if (dma_delay < 0 || send_delay < 0) return 1;
    
    /* calculate delays in cycles */

    nic->dma_delay = hardware->clockspeed / 1000 * dma_delay;
    nic->send_delay = hardware->clockspeed / 1000 * send_delay;
    return 0;
}

/* port 1 command
 * port 4 dmaaddr
 */
int nic_io_write(device_t *dev, uint32_t addr, uint32_t data) {
    nicdevice_t * nic;

    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */

    if (addr >= NIC_IO_LENGTH || (addr & 3))
	return 1;
	
    nic = (nicdevice_t *)dev->realdevice;

    switch(addr) {
    case NIC_PORT_COMMAND:
	/* Clear errors from previous commands */
	nic->status &= 0x87ffffff; 
        switch(data) {
        case NIC_RECEIVE:
            /* transfer from receive buffer to mem */
            if (NIC_VALID_DMA_ADDR(nic)) {
                /* Check that recv buffer contains a frame */
                if (!NIC_STATUS_RXBUSY(nic)) {
                    SET_NIC_STATUS_NOFRAME(nic, 1);
                    return 0;
                }
                /* Check that nic is not busy  */
                if (NIC_STATUS_RBUSY(nic)) {
                    SET_NIC_STATUS_EBUSY(nic, 1);
                    return 0;
                }
                SET_NIC_STATUS_RBUSY(nic, 1);
                memcpy((uint8_t *)hardware->memory->physmem +
                       nic->dma_addr,
                       nic->recv_buffer, nic->mtu);
                /* set pending event */
                nic->recv_event = hardware->cycle_count + nic->dma_delay;
            } else {
                SET_NIC_STATUS_IADDR(nic, 1);
            }
           break;
        case NIC_SEND:
            /* transfer from mem to send buffer */
            if (NIC_VALID_DMA_ADDR(nic)) {

                /* Check that nic is not busy */
                if (NIC_STATUS_SBUSY(nic)) {
                    SET_NIC_STATUS_EBUSY(nic, 1);
                    return 0;
                }

                SET_NIC_STATUS_SBUSY(nic, 1);
                memcpy(nic->send_buffer, 
                       (uint8_t *)hardware->memory->physmem +
                       nic->dma_addr,
                       nic->mtu);

                /* set pending event */

                /* Check for a pending send */
                if (nic->send_event != UINT64_C(0xffffffffffffffff))
                    nic->send_event += nic->dma_delay;
                else
                    nic->send_event = hardware->cycle_count + nic->dma_delay;
            } else {
                SET_NIC_STATUS_IADDR(nic, 1);
            }
            break;
        case NIC_CLEAR_RXIRQ:
            /* clear RXIRQ */
            SET_NIC_STATUS_RXIRQ(nic, 0);
            break;
        case NIC_CLEAR_RIRQ:
            /* clear RIRQ */
            SET_NIC_STATUS_RIRQ(nic, 0);
            break;
        case NIC_CLEAR_SIRQ:
            /* clear SIRQ */
            SET_NIC_STATUS_SIRQ(nic, 0);
            break;
        case NIC_CLEAR_RXBUSY:
            /* clear RXBUSY */
            SET_NIC_STATUS_RXBUSY(nic, 0);
            break;
        case NIC_ENTER_PROMISC:
            /* Enter promiscous mode */
            SET_NIC_STATUS_PROMISC(nic, 1);
            break;
        case NIC_EXIT_PROMISC:
            /* Exit promiscous mode */
            SET_NIC_STATUS_PROMISC(nic, 0);
            break;
        default:
            /* Invalid command */
            nic->status |= 0x20000000; 
        }
	break;
    case NIC_PORT_DMAADDR: 
        nic->dma_addr = data;
	break;
    }

    return 0;
}

/* port 0 status
 * port 2 hwaddr
 * port 3 mtu
 */
int nic_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */

    if (addr >= NIC_IO_LENGTH || (addr & 3))
	return 1;

    if (data != NULL) {
	switch(addr) { 
	case NIC_PORT_STATUS:
            *data = ((nicdevice_t *)dev->realdevice)->status;
	    break;
	case NIC_PORT_HWADDR:
            *data = ntohl(((nicdevice_t *)dev->realdevice)->hwaddr);
	    break;
	case NIC_PORT_MTU:
            *data = (uint32_t)((nicdevice_t *)dev->realdevice)->mtu;
	    break;
	default:
	    *data = 0;
        }

    }
    return 0;
}

static int nic_send_unix(nicdevice_t *nic) {
    DIR * dp;
    struct dirent *ep;
    struct stat st;
    struct sockaddr_un suna;

    /* Find all sockets in dir and multicast */
    dp = opendir(nic->dir);

    if (dp != NULL){
        suna.sun_family = AF_UNIX;
        while ((ep = readdir (dp))) {
            stat(ep->d_name, &st);
            if (S_ISSOCK(st.st_mode)) {
                snprintf(suna.sun_path, sizeof(suna.sun_path), 
                         "%s%s", nic->dir, ep->d_name);
                if(sendto(nic->sock, nic->send_buffer, nic->mtu, 0, 
                          (struct sockaddr *) &suna, sizeof(suna)) < 0) {
                    /* XXX there might be dead or tty sockets in dir */
                    perror("NIC: Send failed");
                }
            }
        }
        closedir (dp);
    }
    else {
        perror ("NIC: Directory open failed while sending");
        SET_NIC_STATUS_ERROR(nic, 1UL);
    }
    return 0;
}

int nic_update(device_t *dev) {
    nicdevice_t * nic;

    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */

    nic = (nicdevice_t *)dev->realdevice;

    if (!(hardware->cycle_count & UINT64_C(0x0000000000000fff))) {
        if (!NIC_STATUS_RXBUSY(nic)) {

            /* Poll socket */
            if (recv(nic->sock,nic->recv_buffer, nic->mtu, 0) >= 0) {
                /* Check  the hw address (in promisc mode, my own addr or
                   broadcast addr)*/
                if(NIC_STATUS_PROMISC(nic) || 
                   ((uint32_t *)nic->recv_buffer)[0] == nic->hwaddr ||
                   ((uint32_t *)nic->recv_buffer)[0] == 0xffffffff) {
                    SET_NIC_STATUS_RXBUSY(nic, 1);
		    /* select a cpu for irq if needed */
		    if(!NIC_STATUS_IRQS(nic))
			nic->cpuirq = select_cpu_for_irq();
                    SET_NIC_STATUS_RXIRQ(nic, 1);

                }
            }

        }
    
        /* Check pending events */
        if (nic->recv_event <= hardware->cycle_count) {
            /* transfer from recv buffer to mem completed */
            SET_NIC_STATUS_RBUSY(nic, 0);
	    /* select a cpu for irq if needed */
	    if(!NIC_STATUS_IRQS(nic))
		nic->cpuirq = select_cpu_for_irq();
            SET_NIC_STATUS_RIRQ(nic, 1);
            nic->recv_event = UINT64_C(0xffffffffffffffff);
        }

        if (nic->send_event <= hardware->cycle_count) {
            if (NIC_STATUS_SBUSY(nic)) {
                /* mem to send buffer transfer complete */

                /* Set the hw addr */
                ((uint32_t *)nic->send_buffer)[1] = nic->hwaddr;
                
                /* Reliability */
                if ((rand() % 100) <= nic->reliability) {

                    if (nic->sock_domain == PF_INET) {
                        if(sendto(nic->sock, nic->send_buffer, nic->mtu, 0, 
                                  (struct sockaddr *) &nic->multicast_addr,
                                  sizeof(nic->multicast_addr)) < 0) {
                            perror("NIC: Send failed");
                            SET_NIC_STATUS_ERROR(nic, 1UL);
                        }
                    } else if (nic->sock_domain == PF_UNIX) {
                        nic_send_unix(nic);
                    } else {
                        printf("NIC is broken :(\n");
                        SET_NIC_STATUS_ERROR(nic, 1UL);
                    }
                }

                SET_NIC_STATUS_SBUSY(nic, 0);
		/* select a cpu for irq if needed */
		if(!NIC_STATUS_IRQS(nic))
		    nic->cpuirq = select_cpu_for_irq();

                SET_NIC_STATUS_SIRQ(nic, 1);

                /* set pending event for completion of send */
                nic->send_event = hardware->cycle_count + nic->send_delay;
            } else {
                /* send completed */
                nic->send_event = UINT64_C(0xffffffffffffffff);
            }
        }
        
    }

    /* Raise interrupt if needed */
    if (NIC_STATUS_IRQS(nic)) {
        /* signal interrupt */
	RAISE_HW_INTERRUPT(nic->cpuirq, dev->irq);
    }

    return 0;
}
