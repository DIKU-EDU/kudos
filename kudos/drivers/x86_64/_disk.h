/*
 * Disk driver
 */

#ifndef KUDOS_DRIVERS_X86_64__DISK_H
#define KUDOS_DRIVERS_X86_64__DISK_H

/* Includes */

/* Defines */
#define IDE_CHANNELS_PER_CTRL   0x2     /* 2 Channels per Controller */
#define IDE_DEVICES_PER_CHANNEL 0x2     /* 2 Devices per Channel (slaver & master) */

/* Default Ports */
#define IDE_PRIMARY_CMD_BASE    0x1F0
#define IDE_PRIMARY_CTRL_BASE   0x3F6
#define IDE_PRIMARY_IRQ         14

#define IDE_SECONDARY_CMD_BASE  0x170
#define IDE_SECONDARY_CTRL_BASE 0x376
#define IDE_SECONDARY_IRQ       15

#define IDE_PRIMARY             0
#define IDE_SECONDARY           1

#define IDE_REGISTER_DATA       0x00
#define IDE_REGISTER_FEATURES   0x01
#define IDE_REGISTER_SECCOUNT0  0x02
#define IDE_REGISTER_LBA0       0x03
#define IDE_REGISTER_LBA1       0x04
#define IDE_REGISTER_LBA2       0x05
#define IDE_REGISTER_HDDSEL     0x06
#define IDE_REGISTER_COMMAND    0x07
#define IDE_REGISTER_STATUS     0x07
#define IDE_REGISTER_CTRL       0x0C

#define IDE_COMMAND_PIO_READ    0x20
#define IDE_COMMAND_PIO_WRITE   0x30
#define IDE_COMMAND_PACKET      0xA1
#define IDE_COMMAND_FLUSH       0xE7
#define IDE_COMMAND_IDENTIFY    0xEC

#define IDE_ERROR_GENERIC       0x01
#define IDE_ERROR_DRQ           0x08
#define IDE_ERROR_FAULT         0x20

#define IDE_ATA_BUSY            0x80
#define IDE_ATA_DRQ             0x08

#define IDE_READ                0x00
#define IDE_WRITE               0x01

/* Structures */

/* IDE Channel */
typedef struct ide_channel_t
{
  /* I/O Base */
  uint16_t base;

  /* Control Base */
  uint16_t ctrl;

  /* Bus Master Base */
  uint16_t busm;

  /* Irq Number */
  uint8_t irq;

  /* Interrupt or Polling? */
  uint32_t use_irq;
  uint32_t use_dma;

  /* Irq waiting? */
  volatile uint32_t irq_wait;

  /* Dma Buffers */
  uint32_t dma_phys;
  uint32_t dma_virt;
  uint32_t dma_buf_phys;
  uint32_t dma_buf_virt;

} ide_channel_t;

/* Ide Device */
typedef struct ide_device_t
{
  /* Driver Present? */
  uint8_t present;
  uint8_t channel;      /* 0 - primary, 1 - secondary */
  uint8_t drive;                /* 0 - master, 1 - slave */

  uint8_t flags;                /* 1 - LBA48, 2 - DMA */

  uint16_t type;                /* 0 - ATA, 1 - ATAPI */
  uint16_t signature;   /* Drive Signature */

  uint16_t capabilities;        /* Features */
  uint32_t commandset;  /* Command set supported */

  uint16_t cylinders;
  uint16_t headspercylinder;
  uint64_t secsperhead;
  uint64_t totalsectors;

} ide_device_t;

#endif // KUDOS_DRIVERS_X86_64__DISK_H
