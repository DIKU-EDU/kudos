/*
 * Disk driver
 */

#include <arch.h>
#include <pci.h>
#include <asm.h>
#include "kernel/stalloc.h"
#include "kernel/panic.h"
#include "kernel/assert.h"
#include "kernel/semaphore.h"
#include "kernel/spinlock.h"
#include "kernel/interrupt.h"
#include "lib/libc.h"
#include "drivers/device.h"
#include "drivers/gbd.h"
#include "drivers/disk.h"
#include "drivers/disksched.h"

extern int device_register(device_t *device);

/**@name Disk driver
 *
 * This module contains functions for disk driver.
 *
 * @{
 */

/* externs */
extern void ide_irq_handler0(void);
extern void ide_irq_handler1(void);

/* Variables */
ide_channel_t ide_channels[IDE_CHANNELS_PER_CTRL];
ide_device_t ide_devices[IDE_CHANNELS_PER_CTRL * IDE_DEVICES_PER_CHANNEL];
device_t ide_dev[IDE_CHANNELS_PER_CTRL * IDE_DEVICES_PER_CHANNEL];
gbd_t ide_gbd[IDE_CHANNELS_PER_CTRL * IDE_DEVICES_PER_CHANNEL];

/* Prototypes */
uint32_t ide_get_sectorsize(gbd_t *disk);
uint32_t ide_get_sectorcount(gbd_t *disk);
int ide_read_block(gbd_t *gbd, gbd_request_t *request);
int ide_write_block(gbd_t *gbd, gbd_request_t *request);

/**
 * Initialize disk device driver. Reserves memory for data structures
 * and register driver to the interrupt handler.
 *
 * @param desc Pointer to the PCI IO device descriptor of the controller
 *
 * @return Pointer to the device structure of the controller
 */
void ide_write(uint8_t channel, uint8_t reg, uint8_t data)
{
  /* */
  if(reg > 0x07 && reg < 0x0C)
    ide_write(channel, IDE_REGISTER_CTRL, 0x80 | ide_channels[channel].irq);

  /* */
  if(reg < 0x08)
    _outb((uint16_t)(ide_channels[channel].base + reg), data);
  else if(reg < 0x0C)
    _outb((uint16_t)(ide_channels[channel].base + reg - 0x06), data);
  else if(reg < 0x0E)
    _outb((uint16_t)(ide_channels[channel].ctrl + reg - 0x0A), data);
  else if(reg < 0x16)
    _outb((uint16_t)(ide_channels[channel].busm + reg - 0x0E), data);

  if(reg > 0x07 && reg < 0x0C)
    ide_write(channel, IDE_REGISTER_CTRL, ide_channels[channel].irq);
}

uint8_t ide_read(uint8_t channel, uint8_t reg)
{
  /* */
  uint8_t data = 0;;

  if(reg > 0x07 && reg < 0x0C)
    ide_write(channel, IDE_REGISTER_CTRL, 0x80 | ide_channels[channel].irq);

  /* */
  if(reg < 0x08)
    data = _inb((uint16_t)(ide_channels[channel].base + reg));
  else if(reg < 0x0C)
    data = _inb((uint16_t)(ide_channels[channel].base + reg - 0x06));
  else if(reg < 0x0E)
    data = _inb((uint16_t)(ide_channels[channel].ctrl + reg - 0x0A));
  else if(reg < 0x16)
    data = _inb((uint16_t)(ide_channels[channel].busm + reg - 0x0E));

  if(reg > 0x07 && reg < 0x0C)
    ide_write(channel, IDE_REGISTER_CTRL, ide_channels[channel].irq);

  return data;
}

void ide_delay(uint8_t channel)
{
  /* Each call delays 100 ns */
  ide_read(channel, IDE_REGISTER_STATUS);
  ide_read(channel, IDE_REGISTER_STATUS);
  ide_read(channel, IDE_REGISTER_STATUS);
  ide_read(channel, IDE_REGISTER_STATUS);
}

/**
 * Initialize disk device driver. Reserves memory for data structures
 * and register driver to the interrupt handler.
 *
 * @param desc Pointer to the PCI IO device descriptor of the controller
 *
 * @return Pointer to the device structure of the controller
 */
int disk_init(io_descriptor_t *desc)
{
  /* Cast it */
  pci_conf_t *pci = (pci_conf_t*)(uint64_t*) desc;

  /* Set bases to default values */
  uint16_t iobase1 = IDE_PRIMARY_CMD_BASE;
  uint16_t iobase2 = IDE_SECONDARY_CMD_BASE;
  uint16_t ctrlbase1 = IDE_PRIMARY_CTRL_BASE;
  uint16_t ctrlbase2 = IDE_SECONDARY_CTRL_BASE;
  uint16_t busmaster = pci->bar4;
  uint32_t i, j, count = 0;
  uint16_t buf[256];

  /* Check for compatability mode */
  if(pci->prog_if & 0x1)
    {
      /* Native mode for channel 1 */
      iobase1 = (uint16_t)pci->bar0;
      ctrlbase1 = (uint16_t)pci->bar1;

      /* Check if they can be relocated */
      if(iobase1 & 0x1)
        iobase1--;

      if(ctrlbase1 & 0x1)
        ctrlbase1--;
    }

  if(pci->prog_if & 0x4)
    {
      /* Native mode for channel 2 */
      iobase2 = (uint16_t)pci->bar2;
      ctrlbase2 = (uint16_t)pci->bar3;

      /* Check if they can be relocated */
      if(iobase2 & 0x1)
        iobase2--;

      if(ctrlbase2 & 0x1)
        ctrlbase2--;
    }

  /* Setup Channels */
  ide_channels[IDE_PRIMARY].busm = busmaster;
  ide_channels[IDE_PRIMARY].base = iobase1;
  ide_channels[IDE_PRIMARY].ctrl = ctrlbase1;
  ide_channels[IDE_PRIMARY].irq = IDE_PRIMARY_IRQ;
  ide_channels[IDE_PRIMARY].irq_wait = 0;
  ide_channels[IDE_PRIMARY].dma_phys = 0;
  ide_channels[IDE_PRIMARY].dma_virt = 0;
  ide_channels[IDE_PRIMARY].dma_buf_phys = 0;
  ide_channels[IDE_PRIMARY].dma_buf_virt = 0;

  ide_channels[IDE_SECONDARY].busm = busmaster;
  ide_channels[IDE_SECONDARY].base = iobase2;
  ide_channels[IDE_SECONDARY].ctrl = ctrlbase2;
  ide_channels[IDE_SECONDARY].irq = IDE_SECONDARY_IRQ;
  ide_channels[IDE_SECONDARY].irq_wait = 0;
  ide_channels[IDE_SECONDARY].dma_phys = 0;
  ide_channels[IDE_SECONDARY].dma_virt = 0;
  ide_channels[IDE_SECONDARY].dma_buf_phys = 0;
  ide_channels[IDE_SECONDARY].dma_buf_virt = 0;

  /* Install interrupts */
  interrupt_register(IDE_PRIMARY_IRQ, (int_handler_t)ide_irq_handler0, 0);
  interrupt_register(IDE_SECONDARY_IRQ, (int_handler_t)ide_irq_handler1, 0);

  /* Disable Irqs, we use polling mode */
  ide_write(IDE_PRIMARY, IDE_REGISTER_CTRL, 2);
  ide_write(IDE_SECONDARY, IDE_REGISTER_CTRL, 2);

  /* Enumerate devices */
  /* We send an IDE_IDENTIFY command to each device, on
   * each channel, and see if it responds */
  for(i = 0; i < IDE_CHANNELS_PER_CTRL; i++)
    {
      for(j = 0; j < IDE_DEVICES_PER_CHANNEL; j++)
        {
          /* Variables */
          uint8_t error = 0, type = 0, status = 0;
          uint32_t lba28 = 0;
          uint64_t lba48 = 0;
          ide_devices[count].present = 0;                       //assume no drive

          /* Step 1. Select drive */
          ide_write(i, IDE_REGISTER_HDDSEL, 0xA0 | (j << 4));
          ide_delay(i);

          /* Step 2. Send IDE_IDENTIFY */
          ide_write(i, IDE_REGISTER_SECCOUNT0, 0);
          ide_write(i, IDE_REGISTER_LBA0, 0);
          ide_write(i, IDE_REGISTER_LBA1, 0);
          ide_write(i, IDE_REGISTER_LBA2, 0);
          ide_write(i, IDE_REGISTER_LBA2, 0);
          ide_write(i, IDE_REGISTER_COMMAND, IDE_COMMAND_IDENTIFY);
          ide_delay(i);

          /* Step 3. Poll */
          status = _inb(ide_channels[i].base + IDE_REGISTER_STATUS);
          if(status == 0 || status == 0x7F || status == 0xFF)
            {
              count++;
              continue;
            }

          /* Wuhuu! device is here */
          while(1)
            {
              status = _inb(ide_channels[i].base + IDE_REGISTER_STATUS);

              if(status & 0x1)
                {
                  error = 1;
                  break;
                }

              if(!(status & IDE_ATA_BUSY) && (status & IDE_ATA_DRQ))
                {
                  break;
                }
            }

          /* Step 4. Probe for ATAPI */
          if(error != 0)
            {
              /* Get type */
              uint8_t cl = _inb(ide_channels[i].base + IDE_REGISTER_LBA1);
              uint8_t ch = _inb(ide_channels[i].base + IDE_REGISTER_LBA2);

              if(cl == 0x14 && ch == 0xEB)              /* PATAPI */
                type = 1;
              else if(cl == 0x69 && ch == 0x96) /* SATAPI */
                type = 1;
              else
                {
                  /* Unknown Type */
                  count++;
                  continue;
                }

              /* Identify */
              ide_write(i, IDE_REGISTER_COMMAND, IDE_COMMAND_PACKET);
              ide_delay(i);
            }

          /* Step 5. Read identification space */
          _insw(ide_channels[i].base + IDE_REGISTER_DATA, 256, (uint8_t*)buf);

          /* Step 6. Read device parameters */
          ide_devices[count].present = 1;
          ide_devices[count].type = type;
          ide_devices[count].channel = i;
          ide_devices[count].drive = j;
          ide_devices[count].signature = (*(uint16_t*)(buf));
          ide_devices[count].capabilities = (*(uint16_t*)(buf + 49));
          ide_devices[count].commandset = (*(uint32_t*)(buf + 82));

          /* Step 7. Get geometry */
          lba28 = (*(uint32_t*)(buf + 60));
          lba48 = (*(uint64_t*)(buf + 100));

          if(lba48)
            {
              ide_devices[count].totalsectors = lba48;
              ide_devices[count].cylinders = (*(uint16_t*)(buf + 1));
              ide_devices[count].headspercylinder = (*(uint16_t*)(buf + 3));
              ide_devices[count].secsperhead = (*(uint64_t*)(buf + 6));
              ide_devices[count].flags |= 0x1;
            }
          else if(lba28 && !lba48)
            {
              ide_devices[count].totalsectors = lba28;
              ide_devices[count].cylinders = (*(uint16_t*)(buf + 1));
              ide_devices[count].headspercylinder = (*(uint16_t*)(buf + 3));
              ide_devices[count].secsperhead = (*(uint64_t*)(buf + 6));
            }
          else
            {
              ide_devices[count].totalsectors = 0;
              ide_devices[count].cylinders = 0;
              ide_devices[count].headspercylinder = 0;
              ide_devices[count].secsperhead = 0;
            }

          /* Register filesystem */
          ide_dev[count].real_device = &(ide_channels[0]);
          ide_dev[count].type = TYPECODE_DISK;
          ide_dev[count].generic_device = &ide_gbd[count];
          ide_dev[count].io_address = count;

          /* Setup ide device */
          ide_gbd[count].device = &ide_dev[count];
          ide_gbd[count].write_block  = ide_write_block;
          ide_gbd[count].read_block   = ide_read_block;
          ide_gbd[count].block_size     = ide_get_sectorsize;
          ide_gbd[count].total_blocks = ide_get_sectorcount;

          device_register(&ide_dev[count]);

          /* Increase count */
          count++;
        }
    }


  return 0;
}

pci_module_init(IDE_PCI_MODULE, disk_init, 0x1, 0x1);

/**
 * Initialize disk device driver. Reserves memory for data structures
 * and register driver to the interrupt handler.
 *
 * @param desc Pointer to the PCI IO device descriptor of the controller
 *
 * @return Pointer to the device structure of the controller
 */
uint32_t ide_get_sectorsize(gbd_t *disk)
{
  /* All */
  device_t *dev = (device_t*)disk->device;
  uint8_t type = ide_devices[(uint8_t)dev->io_address].type;

  if(type == 0)
    return 512; /* Harddisk ? Sector size is 512 */
  else
    return 2048; /* CD/DVD ? Sector size is 2048 */
}

/**
 * Initialize disk device driver. Reserves memory for data structures
 * and register driver to the interrupt handler.
 *
 * @param desc Pointer to the PCI IO device descriptor of the controller
 *
 * @return Pointer to the device structure of the controller
 */
uint32_t ide_get_sectorcount(gbd_t *disk)
{
  /* All */
  device_t *dev = (device_t*)disk->device;
  uint64_t sectors = ide_devices[(uint8_t)dev->io_address].totalsectors;

  return (uint32_t)sectors;
}

int32_t ide_wait(uint8_t channel, int advanced)
{
  uint8_t status = 0;

  /* Do the delay first */
  ide_delay(channel);

  /* Now we wait for it to clear BUSY */
  while((status = _inb(ide_channels[channel].base + IDE_REGISTER_STATUS))
        & IDE_ATA_BUSY);

  /* Do error check aswell? */
  if(advanced)
    {
      status = _inb(ide_channels[channel].base + IDE_REGISTER_STATUS);

      if(status & IDE_ERROR_FAULT)
        return -1;
      if(status & IDE_ERROR_GENERIC)
        return -2;
      if(!(status & IDE_ERROR_DRQ))
        return -3;
    }

  return 0;
}

int32_t ide_pio_readwrite(uint8_t rw, uint8_t drive, uint64_t lba, 
                          uint8_t *buf, uint32_t numsectors)
{
  /* Sanity */
  if(rw > 1 || buf == 0)
    return 0;

  /* Vars */
  uint64_t addr = lba;
  uint8_t lba_mode = 1; /* 1 - 28, 2 - 48 */
  uint8_t cmd = 0;
  uint8_t channel = ide_devices[drive].channel;
  uint32_t slave = ide_devices[drive].drive;
  uint32_t bus = ide_channels[channel].base;
  uint32_t words = (numsectors * 512) / 2;

  /* Make sure IRQs are disabled */
  _outb(bus + IDE_REGISTER_CTRL, 0x02);

  /* Wait for it to acknowledge */
  ide_wait(channel, 0);

  /* Determine LBA mode */
  if(ide_devices[drive].flags & 0x1)
    lba_mode = 2;

  /* Read or write? */
  if(rw == IDE_READ)
    cmd = IDE_COMMAND_PIO_READ;
  else
    cmd = IDE_COMMAND_PIO_WRITE;

  /* Reset IRQ counter */
  ide_channels[channel].irq_wait = 0;

  /* Now, send the command */
  if(lba_mode == 2)
    {
      /* LBA48 */
      cmd += 0x04;

      /* Send it */
      ide_write(channel, IDE_REGISTER_HDDSEL, (0x40 | (slave << 4)));
      ide_wait(channel, 0);
      ide_write(channel, IDE_REGISTER_SECCOUNT0, 0x00);
      ide_write(channel, IDE_REGISTER_LBA0, (uint8_t)((addr >> 24) & 0xFF));
      ide_write(channel, IDE_REGISTER_LBA1, (uint8_t)((addr >> 32) & 0xFF));
      ide_write(channel, IDE_REGISTER_LBA2, (uint8_t)((addr >> 40) & 0xFF));
    }
  else if(lba_mode == 1)
    {
      /* LBA28 */
      ide_write(channel, IDE_REGISTER_HDDSEL, 
                0xE0 | (slave << 4) | ((addr & 0x0F000000) >> 24));
      ide_wait(channel, 0);
      ide_write(channel, IDE_REGISTER_FEATURES, 0x00);
    }

  /* Send (rest) of command */
  ide_write(channel, IDE_REGISTER_SECCOUNT0, (uint8_t)numsectors);
  ide_write(channel, IDE_REGISTER_LBA0, (uint8_t)(addr & 0xFF));
  ide_write(channel, IDE_REGISTER_LBA1, (uint8_t)((addr >> 8) & 0xFF));
  ide_write(channel, IDE_REGISTER_LBA2, (uint8_t)((addr >> 16) & 0xFF));

  /* Command time */
  ide_write(channel, IDE_REGISTER_COMMAND, cmd);

  /* Make sure command finished */
  if(ide_wait(channel, 1) != 0)
    {
      kprintf("ide_read_write: Error!");
      return 0;
    }

  /* Ok all is fine, now its time to read! */
  if(rw == IDE_READ)
    {
      /* Read */
      _insw(bus + IDE_REGISTER_DATA, (256 * numsectors), buf);
    }
  else
    {
      /* Write it */
      _outsw(bus + IDE_REGISTER_DATA, (256 * numsectors), buf);

      /* Flush */
      _outb(bus + IDE_REGISTER_COMMAND, IDE_COMMAND_FLUSH);
    }

  /* Delay, wait for drive to finish */
  ide_wait(channel, 0);

  return (words * 2);
}

int ide_read_block(gbd_t *gbd, gbd_request_t *request)
{
  /* Get disk */
  device_t *disk = (device_t*)gbd->device;
  uint8_t drive = (uint8_t)disk->io_address;
  uint8_t *buf = (uint8_t*)(uint64_t)request->buf;
  uint64_t sector = (uint64_t)request->block;

  /* Sanity checks */
  if(drive > 3 || ide_devices[drive].present == 0)
    return -6;
  if(sector > ide_devices[drive].totalsectors ||
     ide_devices[drive].type != 0)
    return -5;

  /* Ok things seems in order, gogo */
  return ide_pio_readwrite(IDE_READ, drive, sector, buf, 1);
}

int ide_write_block(gbd_t *gbd, gbd_request_t *request)
{
  /* Get disk */
  device_t *disk = (device_t*)gbd->device;
  uint8_t drive = (uint8_t)disk->io_address;
  uint8_t *buf = (uint8_t*)(uint64_t)request->buf;
  uint64_t sector = (uint64_t)request->block;

  /* Sanity checks */
  if(drive > 3 || ide_devices[drive].present == 0)
    return -1;
  if(sector > ide_devices[drive].totalsectors ||
     ide_devices[drive].type != 0)
    return -2;

  /* Ok things seem in order, gogo */
  return ide_pio_readwrite(IDE_WRITE, drive, sector, buf, 1);
}

/** @} */
