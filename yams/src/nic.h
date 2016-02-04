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

   $Id: nic.h,v 1.9 2002/11/11 15:28:32 tlilja Exp $
*/
#ifndef YAMS_NIC_H
#define YAMS_NIC_H

/* nic ports */
#define NIC_PORT_STATUS  0x00
#define NIC_PORT_COMMAND 0x04
#define NIC_PORT_HWADDR  0x08
#define NIC_PORT_MTU     0x0c
#define NIC_PORT_DMAADDR 0x10

#define NIC_IO_LENGTH    0x14

/*nic commands */

#define NIC_RECEIVE       0x01
#define NIC_SEND          0x02
#define NIC_CLEAR_RXIRQ   0x03
#define NIC_CLEAR_RIRQ    0x04
#define NIC_CLEAR_SIRQ    0x05
#define NIC_CLEAR_RXBUSY  0x06
#define NIC_ENTER_PROMISC 0x07
#define NIC_EXIT_PROMISC  0x08

#define NIC_VALID_DMA_ADDR(nic) \
    (((nic)->dma_addr + (nic)->mtu) < \
        (hardware->memory->num_pages * hardware->memory->pagesize))

#define NIC_STATUS_IRQS(nic) \
    (((nic)->status & 0x00000038) >> 3)

/* get macros for nic status */
#define NIC_STATUS_RXBUSY(nic) \
   ((nic)->status & 0x00000001)

#define NIC_STATUS_RBUSY(nic) \
   (((nic)->status & 0x00000002) >> 1)

#define NIC_STATUS_SBUSY(nic) \
   (((nic)->status & 0x00000004) >> 2)

#define NIC_STATUS_RXIRQ(nic) \
   (((nic)->status & 0x00000008) >> 3)

#define NIC_STATUS_RIRQ(nic) \
   (((nic)->status & 0x00000010) >> 4)

#define NIC_STATUS_SIRQ(nic) \
   (((nic)->status & 0x00000020) >> 5)

#define NIC_STATUS_PROMISC(nic) \
   (((nic)->status & 0x00000040) >> 6)

/* set macros for nic status */
#define SET_NIC_STATUS_RXBUSY(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000001) | ((value) & 0x00000001)

#define SET_NIC_STATUS_RBUSY(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000002) | \
   (((value) & 0x00000001) << 1)

#define SET_NIC_STATUS_SBUSY(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000004) | \
   (((value) & 0x00000001) << 2)

#define SET_NIC_STATUS_RXIRQ(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000008) | \
   (((value) & 0x00000001) << 3)

#define SET_NIC_STATUS_RIRQ(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000010) | \
   (((value) & 0x00000001) << 4)

#define SET_NIC_STATUS_SIRQ(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000020) | \
   (((value) & 0x00000001) << 5)

#define SET_NIC_STATUS_PROMISC(nic,value) \
   (nic)->status = ((nic)->status & ~0x00000040) | \
   (((value) & 0x00000001) << 6)

#define SET_NIC_STATUS_NOFRAME(nic,value) \
   (nic)->status = ((nic)->status & ~0x08000000) | \
   (((value) & 0x00000001) << 27)

#define SET_NIC_STATUS_IADDR(nic,value) \
   (nic)->status = ((nic)->status & ~0x10000000) | \
   (((value) & 0x00000001) << 28)

#define SET_NIC_STATUS_ICOMM(nic,value) \
   (nic)->status = ((nic)->status & ~0x20000000) | \
   (((value) & 0x00000001) << 29)

#define SET_NIC_STATUS_EBUSY(nic,value) \
   (nic)->status = ((nic)->status & ~0x40000000) | \
   (((value) & 0x00000001) << 30)

#define SET_NIC_STATUS_ERROR(nic,value) \
   (nic)->status = ((nic)->status & ~0x80000000) | \
   (((value) & 0x00000001) << 31)

/* NIC device */

#define IRQ_NIC 3
#define IOLENGTH_NIC 20

typedef struct {
    int mtu, reliability, send_delay, dma_delay;
    uint32_t hwaddr;
    int sock;
    char *sock_name;
    int sock_port;
    int sock_domain;
    struct sockaddr_in multicast_addr;
    char *dir;

    uint32_t dma_addr;
    uint32_t status;

    uint8_t * recv_buffer;
    uint8_t * send_buffer;

    uint64_t send_event;
    uint64_t recv_event;

    int cpuirq;
} nicdevice_t;

/* Allocates memory for a disk device */
device_t *nic_create();
void nic_destroy(device_t *nic);

/* Creates a new NIC device
 * domain: either PF_INET or PF_UNIX
 * name: PF_UNIX: name of the socket file
 *       PF_INET: IP address of the multicast group
 * port: multicast port to listen, ignored if PF_UNIX
 *
 * returns:
 * 0 - OK
 * 1 - bind() failed for the socket
 * 2 - invalid parameters
 */
int nic_init(int domain, char *name, int port, int mtu,
	     device_t *nic);

/* returns: 0 - OK, 1 - invalid parameters (e.g. rel not in 0..100) */
int nic_setreliability(int reliability, device_t *nic);

/* returns: 0 - OK, 1 - invalid parameters */
int nic_sethwaddr(uint32_t hwaddr, device_t *nic);

/* returns: 0 - OK, 1 - invalid parameters */
int nic_setdelays(int dma_delay, int send_delay, device_t *nic);

int nic_io_write(device_t *dev, uint32_t addr, uint32_t data);
int nic_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int nic_update(device_t *dev);


#endif /* YAMS_NIC_H */
