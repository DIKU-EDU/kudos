/*
 * List of available filesystem drivers.
 */

#include "fs/filesystems.h"
#include "fs/tfs.h"
#include "drivers/device.h"

/* Structure of a partition */
typedef struct partition
{
  uint8_t active;
  uint8_t start_head;
  uint16_t start_cs;
  uint8_t type;
  uint8_t end_head;
  uint16_t end_cs;
  uint32_t start_sector;
  uint32_t size;
} __attribute__((packed)) partition_t;

/* Structure of a boot sector */
typedef struct boot_sector
{
  /* Boot Code */
  uint8_t         bootcode[0x1BE];
  partition_t partitions[4];
  uint16_t        bootsig;

} __attribute__((packed)) boot_sector_t;

/* NULL terminated table of all available filesystems. */

static filesystems_t filesystems[] = {
  {"TFS", &tfs_init},
  { NULL, NULL} /* Last entry must be a NULL pair. */
};


/**
 *
 *
 *
 *
 */
uint32_t get_partition(gbd_t *disk, gbd_request_t *dreq, uint8_t part_num,
                       uint8_t *pactive, uint8_t *ptype, uint32_t *psize,
                       uint64_t mbr_sector)
{
  int bytes_read = 0;
  boot_sector_t *bootsector = 0;

  /* Sanity */
  if(part_num > 3)
    return 0xFFFFFFFF;

  /* Modify request */
  dreq->block = mbr_sector;

  /* Read */
  bytes_read = disk->read_block(disk, dreq);

  /* Sanity */
  if(bytes_read == 0 || bytes_read < 0)
    return 0xFFFFFFFF;

  /* Cast sector read to partition table */
  bootsector = (boot_sector_t*)ADDR_PHYS_TO_KERNEL((uintptr_t)dreq->buf);

  /* Get info */
  if(pactive != 0)
    *pactive = bootsector->partitions[part_num].active;
  if(ptype != 0)
    *ptype = bootsector->partitions[part_num].type;
  if(psize != 0)
    *psize = bootsector->partitions[part_num].size;

  /* Return start sector */
  return bootsector->partitions[part_num].start_sector;
}

/**
 *
 *
 *
 *
 */
fs_t *read_extended_partition_table(gbd_t *disk, gbd_request_t *dreq,
                                    uint32_t start, uint32_t sector)
{
  filesystems_t *driver;
  fs_t *fs = NULL;
  uint8_t i;
  for(i = 0; i < 4; i++)
    {
      uint32_t pstart = 0, psize = 0;
      uint8_t pactive = 0, ptype = 0;

      /* Get partition information */
      pstart = get_partition(disk, dreq, i, &pactive, &ptype,
                             &psize, (uint64_t)(start + sector));

      /* Sanity checks */
      if(pstart == 0xFFFFFFFF || (pactive != 0x80 && pactive != 0x00))
        continue;

      /* Check for EOT */
      if(ptype == 0x00)
        break;

      /* Check partition type */
      /* 0x05 & 0x0F is anoter partition table */
      if(ptype == 0x05 || ptype == 0x0F)
        {
          fs = read_extended_partition_table(disk, dreq, start, pstart);
          break;
        }

      /* Ok, this is some kind of valid partition
       * We now brute-force all fs we know */
      for(driver=filesystems; driver->name != NULL; driver++)
        {
          fs=driver->init(disk, (start + sector + pstart));
          if(fs!=NULL) {
            /* Init succeeded. */
            kprintf("VFS: %s initialized on disk at 0x%8.8x at ext partition %u\n",
                    driver->name, disk->device->io_address, (uint32_t)i);
            return fs;
          }
        }
    }

  /* None found */
  return fs;
}

/**
 * Tries to mount given GBD (disk) with all available filesystem drivers.
 *
 * @param disk Generic Block Device on which a filesystem is supposed to be.
 *
 * @return Initialized filesystem (first match), or NULL if all mount
 * attempts failed.
 */

fs_t *filesystems_try_all(gbd_t *disk)
{
  filesystems_t *driver;
  fs_t *fs = NULL;
  uint8_t i;

  /* Go through partitions on disk if any avail */
  physaddr_t addr = kmalloc(4096);
  gbd_request_t req;

  /* Setup disk request */
  req.block = 0;
  req.sem = NULL;
  req.buf = ADDR_KERNEL_TO_PHYS(addr);   /* disk needs physical addr */

  /* Check for DOS-style partitions, 4 per table.  This works on x86
     and MIPS systems that use DOS partition tables for some
     reason. */
  for(i = 0; i < 4; i++)
    {
      uint32_t pstart = 0, psize = 0;
      uint8_t pactive = 0, ptype = 0;

      /* Get partition information */
      pstart = get_partition(disk, &req, i, &pactive, &ptype, &psize, 0);

      /* Sanity checks */
      if(pstart == 0xFFFFFFFF || (pactive != 0x80 && pactive != 0x00))
        continue;

      /* Check for EOT */
      if(ptype == 0x00)
        break;

      /* Check partition type */
      /* 0x05 & 0x0F is anoter partition table */
      if(ptype == 0x05 || ptype == 0x0F)
        {
          fs = read_extended_partition_table(disk, &req, pstart, 0);
          break;
        }

      /* Ok, this is some kind of valid partition
       * We now brute-force all fs we know */
      for(driver=filesystems; driver->name != NULL; driver++)
        {
          fs=driver->init(disk, pstart);
          if(fs!=NULL) {
            /* Init succeeded. */
            kprintf("VFS: %s initialized on disk at 0x%8.8x at partition %u\n",
                    driver->name, disk->device->io_address, (uint32_t)i);
            goto end;
          }
        }
    }

  /* No match, check if disk one large partition.  This works in
     YAMS. */
  if(fs == NULL)
    {
      for(driver=filesystems; driver->name != NULL; driver++)
        {
          fs=driver->init(disk, 0);
          if(fs!=NULL) {
            /* Init succeeded. */
            kprintf("VFS: %s initialized on disk at 0x%8.8x\n",
                    driver->name, disk->device->io_address);
            goto end;
          }
        }
    }
  /* No match. */

 end:
  physmem_freeblock((void*)addr);
  return fs;
}
