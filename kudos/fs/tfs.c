/*
 * Trivial Filesystem (TFS).
 */

#include "kernel/stalloc.h"
#include "kernel/assert.h"
#include "vm/memory.h"
#include "drivers/gbd.h"
#include "fs/vfs.h"
#include "fs/tfs.h"
#include "lib/libc.h"
#include "lib/bitmap.h"

/**@name Trivial Filesystem (TFS)
 *
 * This module contains implementation for TFS.
 *
 * @{
 */


/* Data structure used internally by TFS filesystem. This data structure
   is used by tfs-functions. it is initialized during tfs_init(). Also
   memory for the buffers is reserved _dynamically_ during init.

   Buffers are used when reading/writing system or data blocks from/to
   disk.
*/
typedef struct {
  /* Total number of blocks of the disk */
  uint32_t       startblock;
  uint32_t       totalblocks;

  /* Pointer to gbd device performing tfs */
  gbd_t          *disk;

  /* lock for mutual exclusion of fs-operations (we support only
     one operation at a time in any case) */
  semaphore_t    *lock;

  /* Buffers for read/write operations on disk. */
  tfs_inode_t    *buffer_inode;   /* buffer for inode blocks */
  bitmap_t       *buffer_bat;     /* buffer for allocation block */
  tfs_direntry_t *buffer_md;      /* buffer for directory block */
} tfs_t;

/**
 * Initialize trivial filesystem. Allocates 1 page of memory dynamically for
 * filesystem data structure, tfs data structure and buffers needed.
 * Sets fs_t and tfs_t fields. If initialization is succesful, returns
 * pointer to fs_t data structure. Else NULL pointer is returned.
 *
 * @param Pointer to gbd-device performing tfs.
 *
 * @return Pointer to the filesystem data structure fs_t, if fails
 * return NULL.
 */
fs_t * tfs_init(gbd_t *disk, uint32_t sector)
{
  physaddr_t addr;
  gbd_request_t req;
  uint16_t magic;
  char name[TFS_VOLNAME_MAX];
  fs_t *fs;
  tfs_t *tfs;
  int r;
  semaphore_t *sem;

  if(disk->block_size(disk) != TFS_BLOCK_SIZE)
    return NULL;

  /* check semaphore availability before memory allocation */
  sem = semaphore_create(1);
  if (sem == NULL) {
    kprintf("tfs_init: could not create a new semaphore.\n");
    return NULL;
  }

  addr = kmalloc(4096);

  if(addr == 0) {
    semaphore_destroy(sem);
    kprintf("tfs_init: could not allocate memory.\n");
    return NULL;
  }
  addr = ADDR_PHYS_TO_KERNEL(addr);      /* transform to vm address */

  /* Assert that one page is enough */
  KERNEL_ASSERT(PAGE_SIZE >= (3*TFS_BLOCK_SIZE+sizeof(tfs_t)+sizeof(fs_t)));

  /* Read header block, and make sure this is tfs drive */
  req.block = sector + TFS_HEADER_BLOCK;
  req.sem = NULL;
  req.buf = ADDR_KERNEL_TO_PHYS(addr);   /* disk needs physical addr */

  r = disk->read_block(disk, &req);
  if(r == 0) {
    semaphore_destroy(sem);
    //NEED kfree function here
    //physmem_freeblock((physaddr_t*)ADDR_KERNEL_TO_PHYS(addr));
    kprintf("tfs_init: Error during disk read. Initialization failed.\n");
    return NULL;
  }

  /* Get magic */
  magic = from_big_endian32((*(uintptr_t*)addr));

  if(magic != TFS_MAGIC) {
    semaphore_destroy(sem);
    //NEED kfree function here
    //physmem_freeblock((physaddr_t*)ADDR_KERNEL_TO_PHYS(addr));
    return NULL;
  }

  /* Copy volume name from header block. */
  stringcopy(name, (char *)(addr+4), TFS_VOLNAME_MAX);

  /* fs_t, tfs_t and all buffers in tfs_t fit in one page, so obtain
     addresses for each structure and buffer inside the allocated
     memory page. */
  fs  = (fs_t *)addr;
  tfs = (tfs_t *)(addr + sizeof(fs_t));
  tfs->buffer_inode = (tfs_inode_t *)(uintptr_t)((uintptr_t)tfs + sizeof(tfs_t));
  tfs->buffer_bat  = (bitmap_t *)(uintptr_t)((uintptr_t)tfs->buffer_inode +
                                          TFS_BLOCK_SIZE);
  tfs->buffer_md   = (tfs_direntry_t *)((uintptr_t)tfs->buffer_bat +
                                        TFS_BLOCK_SIZE);

  tfs->startblock  = sector;
  tfs->totalblocks = MIN(disk->total_blocks(disk), 8*TFS_BLOCK_SIZE);
  tfs->disk        = disk;

  /* save the semaphore to the tfs_t */
  tfs->lock = sem;

  fs->internal = (void *)tfs;
  stringcopy(fs->volume_name, name, VFS_NAME_LENGTH);

  fs->unmount = tfs_unmount;
  fs->open    = tfs_open;
  fs->close   = tfs_close;
  fs->create  = tfs_create;
  fs->remove  = tfs_remove;
  fs->read    = tfs_read;
  fs->write   = tfs_write;
  fs->getfree  = tfs_getfree;
  fs->filecount = tfs_filecount;
  fs->file      = tfs_file;

  return fs;
}


/**
 * Unmounts tfs filesystem from gbd device. After this TFS-driver and
 * gbd-device are no longer linked together. Implements
 * fs.unmount(). Waits for the current operation(s) to finish, frees
 * reserved memory and returns OK.
 *
 * @param fs Pointer to fs data structure of the device.
 *
 * @return VFS_OK
 */
int tfs_unmount(fs_t *fs)
{
  tfs_t *tfs;

  tfs = (tfs_t *)fs->internal;

  semaphore_P(tfs->lock); /* The semaphore should be free at this
                             point, we get it just in case something has gone wrong. */

  /* free semaphore and allocated memory */
  semaphore_destroy(tfs->lock);
  //NEED kfree function here
  //physmem_freeblock((void*)(uintptr_t)ADDR_KERNEL_TO_PHYS((uintptr_t)fs));
  return VFS_OK;
}


/**
 * Opens file. Implements fs.open(). Reads directory block of tfs
 * device and finds given file. Returns file's inode block number or
 * VFS_NOT_FOUND, if file not found.
 *
 * @param fs Pointer to fs data structure of the device.
 * @param filename Name of the file to be opened.
 *
 * @return If file found, return inode block number as fileid, otherwise
 * return VFS_NOT_FOUND.
 */
int tfs_open(fs_t *fs, char *filename)
{
  tfs_t *tfs;
  gbd_request_t req;
  uint32_t i;
  int r;

  tfs = (tfs_t *)fs->internal;

  semaphore_P(tfs->lock);

  req.block     = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf       = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem       = NULL;
  r = tfs->disk->read_block(tfs->disk,&req);
  if(r == 0) {
    /* An error occured during read. */
    kprintf("tfs_open: read error at block 0x%x\n", TFS_DIRECTORY_BLOCK);
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  for(i=0;i < TFS_MAX_FILES;i++) {
    if(stringcmp(tfs->buffer_md[i].name, filename) == 0) {
      semaphore_V(tfs->lock);
      return from_big_endian32(tfs->buffer_md[i].inode);
    }
  }
  kprintf("tfs_open: file not found\n");
  semaphore_V(tfs->lock);
  return VFS_NOT_FOUND;
}


/**
 * Closes file. Implements fs.close(). There is nothing to be done, no
 * data strucutures or similar are reserved for file. Returns VFS_OK.
 *
 * @param fs Pointer to fs data structure of the device.
 * @param fileid File id (inode block number) of the file.
 *
 * @return VFS_OK
 */
int tfs_close(fs_t *fs, int fileid)
{
  fs = fs;
  fileid = fileid;

  return VFS_OK;
}


/**
 * Creates file of given size. Implements fs.create(). Checks that
 * file name doesn't allready exist in directory block.Allocates
 * enough blocks from the allocation block for the file (1 for inode
 * and then enough for the file of given size). Reserved blocks are zeroed.
 *
 * @param fs Pointer to fs data structure of the device.
 * @param filename File name of the file to be created
 * @param size Size of the file to be created
 *
 * @return If file allready exists or not enough space return VFS_ERROR,
 * otherwise return VFS_OK.
 */
int tfs_create(fs_t *fs, char *filename, int size)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  gbd_request_t req;
  uint32_t i;
  uint32_t numblocks = (size + TFS_BLOCK_SIZE - 1)/TFS_BLOCK_SIZE;
  int index = -1;
  int r;

  semaphore_P(tfs->lock);

  if(numblocks > (TFS_BLOCK_SIZE / 4 - 1)) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* Read directory block. Check that file doesn't allready exist and
     there is space left for the file in directory block. */
  req.block = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  for(i=0;i<TFS_MAX_FILES;i++) {
    if(stringcmp(tfs->buffer_md[i].name, filename) == 0) {
      semaphore_V(tfs->lock);
      return VFS_ERROR;
    }

    if(from_big_endian32(tfs->buffer_md[i].inode) == 0) {
      /* found free slot from directory */
      index = i;
    }
  }

  if(index == -1) {
    /* there was no space in directory, because index is not set */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  stringcopy(tfs->buffer_md[index].name,filename, TFS_FILENAME_MAX);

  /* Read allocation block and... */
  req.block = tfs->startblock + TFS_ALLOCATION_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r==0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }


  /* ...find space for inode... */
  tfs->buffer_md[index].inode = bitmap_findnset(tfs->buffer_bat,
                                                tfs->totalblocks);

  if((int)(from_big_endian32(tfs->buffer_md[index].inode)) == -1) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* ...and the rest of the blocks. Mark found block numbers in
     inode.*/
  tfs->buffer_inode->filesize = from_big_endian32(size);
  for(i=0; i<numblocks; i++) {
    tfs->buffer_inode->block[i] = from_big_endian32(bitmap_findnset(tfs->buffer_bat,
                                                                    tfs->totalblocks));
    if((int)from_big_endian32(tfs->buffer_inode->block[i]) == -1) {
      /* Disk full. No free block found. */
      semaphore_V(tfs->lock);
      return VFS_ERROR;
    }
  }

  /* Mark rest of the blocks in inode as unused. */
  while(i < (TFS_BLOCK_SIZE / 4 - 1))
    tfs->buffer_inode->block[i++] = 0;

  req.block = tfs->startblock + TFS_ALLOCATION_BLOCK;
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem   = NULL;
  r = tfs->disk->write_block(tfs->disk, &req);
  if(r==0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  req.block = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem   = NULL;
  r = tfs->disk->write_block(tfs->disk, &req);
  if(r==0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  req.block = tfs->startblock + from_big_endian32(tfs->buffer_md[index].inode);
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_inode);
  req.sem   = NULL;
  r = tfs->disk->write_block(tfs->disk, &req);
  if(r==0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* Write zeros to the reserved blocks. Buffer for allocation block
     is no longer needed, so lets use it as zero buffer. */
  memoryset(tfs->buffer_bat, 0, TFS_BLOCK_SIZE);
  for(i=0;i<numblocks;i++) {
    req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[i]);
    req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
    req.sem   = NULL;
    r = tfs->disk->write_block(tfs->disk, &req);
    if(r==0) {
      /* An error occured. */
      semaphore_V(tfs->lock);
      return VFS_ERROR;
    }

  }

  semaphore_V(tfs->lock);
  return VFS_OK;
}

/**
 * Removes given file. Implements fs.remove(). Frees blocks allocated
 * for the file and directory entry.
 *
 * @param fs Pointer to fs data structure of the device.
 * @param filename file to be removed.
 *
 * @return VFS_OK if file succesfully removed. If file not found
 * VFS_NOT_FOUND.
 */
int tfs_remove(fs_t *fs, char *filename)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  gbd_request_t req;
  uint32_t i;
  int index = -1;
  int r;

  semaphore_P(tfs->lock);

  /* Find file and inode block number from directory block.
     If not found return VFS_NOT_FOUND. */
  req.block = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  for(i=0;i<TFS_MAX_FILES;i++) {
    if(stringcmp(tfs->buffer_md[i].name, filename) == 0) {
      index = i;
      break;
    }
  }
  if(index == -1) {
    semaphore_V(tfs->lock);
    return VFS_NOT_FOUND;
  }

  /* Read allocation block of the device and inode block of the file.
     Free reserved blocks (marked in inode) from allocation block. */
  req.block = tfs->startblock + TFS_ALLOCATION_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  req.block = tfs->startblock + from_big_endian32(tfs->buffer_md[index].inode);
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_inode);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }


  bitmap_set(tfs->buffer_bat, from_big_endian32(tfs->buffer_md[index].inode),0);
  i=0;
  while(from_big_endian32(tfs->buffer_inode->block[i]) != 0 &&
        i < (TFS_BLOCK_SIZE / 4 - 1)) {
    bitmap_set(tfs->buffer_bat, from_big_endian32(tfs->buffer_inode->block[i]),0);
    i++;
  }

  /* Free directory entry. */
  tfs->buffer_md[index].inode   = 0;
  tfs->buffer_md[index].name[0] = 0;

  req.block = tfs->startblock + TFS_ALLOCATION_BLOCK;
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem   = NULL;
  r = tfs->disk->write_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  req.block = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem   = NULL;
  r = tfs->disk->write_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  semaphore_V(tfs->lock);
  return VFS_OK;
}


/**
 * Reads at most bufsize bytes from file to the buffer starting from
 * the offset. bufsize bytes is always read if possible. Returns
 * number of bytes read. Buffer size must be atleast bufsize.
 * Implements fs.read().
 *
 * @param fs  Pointer to fs data structure of the device.
 * @param fileid Fileid of the file.
 * @param buffer Pointer to the buffer the data is read into.
 * @param bufsize Maximum number of bytes to be read.
 * @param offset Start position of reading.
 *
 * @return Number of bytes read into buffer, or VFS_ERROR if error
 * occured.
 */
int tfs_read(fs_t *fs, int fileid, void *buffer, int bufsize, int offset)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  gbd_request_t req;
  int b1, b2;
  int read=0;
  int r;

  semaphore_P(tfs->lock);

  /* fileid is blocknum so ensure that we don't read system blocks
     or outside the disk */

  if(fileid < 2 || fileid > (int)tfs->totalblocks) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  req.block = tfs->startblock + fileid;
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_inode);
  req.sem   = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* Check that offset is inside the file */
  if(offset < 0 || offset > (int)from_big_endian32(tfs->buffer_inode->filesize)) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* Read at most what is left from the file. */
  bufsize = MIN(bufsize,((int)from_big_endian32(tfs->buffer_inode->filesize)) - offset);

  if(bufsize==0) {
    semaphore_V(tfs->lock);
    return 0;
  }

  /* first block to be read from the disk */
  b1 = offset / TFS_BLOCK_SIZE;

  /* last block to be read from the disk */
  b2 = (offset+bufsize-1) / TFS_BLOCK_SIZE;

  /* Read blocks from b1 to b2. First and last are
     special cases because whole block might not be written
     to the buffer. */
  req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[b1]);
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem   = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* Count the number of the bytes to be read from the block and
     written to the buffer from the first block. */
  read = MIN(TFS_BLOCK_SIZE - (offset % TFS_BLOCK_SIZE), bufsize);
  memcopy(read,
          buffer,
          (const uint32_t *)(((uintptr_t)tfs->buffer_bat) +
                             (offset % TFS_BLOCK_SIZE)));

  buffer = (void *)((uintptr_t)buffer + read);
  b1++;
  while(b1 <= b2) {
    req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[b1]);
    req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
    req.sem   = NULL;
    r = tfs->disk->read_block(tfs->disk, &req);
    if(r == 0) {
      /* An error occured. */
      semaphore_V(tfs->lock);
      return VFS_ERROR;
    }

    if(b1 == b2) {
      /* Last block. Whole block might not be read.*/
      memcopy(bufsize - read,
              buffer,
              (const uint32_t *)(uintptr_t)tfs->buffer_bat);
      read += (bufsize - read);
    }
    else {
      /* Read whole block */
      memcopy(TFS_BLOCK_SIZE,
              buffer,
              (const uint32_t *)tfs->buffer_bat);
      read += TFS_BLOCK_SIZE;
      buffer = (void *)((uintptr_t)buffer + TFS_BLOCK_SIZE);
    }
    b1++;
  }

  semaphore_V(tfs->lock);
  return read;
}



/**
 * Write at most datasize bytes from buffer to the file starting from
 * the offset. datasize bytes is always written if possible. Returns
 * number of bytes written. Buffer size must be atleast datasize.
 * Implements fs.read().
 *
 * @param fs  Pointer to fs data structure of the device.
 * @param fileid Fileid of the file.
 * @param buffer Pointer to the buffer the data is written from.
 * @param datasize Maximum number of bytes to be written.
 * @param offset Start position of writing.
 *
 * @return Number of bytes written into buffer, or VFS_ERROR if error
 * occured.
 */
int tfs_write(fs_t *fs, int fileid, void *buffer, int datasize, int offset)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  gbd_request_t req;
  int b1, b2;
  int written=0;
  int r;

  semaphore_P(tfs->lock);

  /* fileid is blocknum so ensure that we don't read system blocks
     or outside the disk */
  if(fileid < 2 || fileid > (int)tfs->totalblocks) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  req.block = tfs->startblock + fileid;
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_inode);
  req.sem   = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* check that start position is inside the disk */
  if(offset < 0 || offset > (int)from_big_endian32(tfs->buffer_inode->filesize)) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  /* write at most the number of bytes left in the file */
  datasize = MIN(datasize,(int)from_big_endian32(tfs->buffer_inode->filesize)-offset);

  if(datasize==0) {
    semaphore_V(tfs->lock);
    return 0;
  }

  /* first block to be written into */
  b1 = offset / TFS_BLOCK_SIZE;

  /* last block to be written into */
  b2 = (offset+datasize-1) / TFS_BLOCK_SIZE;

  /* Write data to blocks from b1 to b2. First and last are special
     cases because whole block might not be written. Because of possible
     partial write, first and last block must be read before writing.

     If we write less than block size or start writing in the middle
     of the block, read the block firts. Buffer for allocation block
     is used because it is not needed (for allocation block) in this
     function. */
  written = MIN(TFS_BLOCK_SIZE - (offset % TFS_BLOCK_SIZE),datasize);
  if(written < TFS_BLOCK_SIZE) {
    req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[b1]);
    req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
    req.sem   = NULL;
    r = tfs->disk->read_block(tfs->disk, &req);
    if(r == 0) {
      /* An error occured. */
      semaphore_V(tfs->lock);
      return VFS_ERROR;
    }
  }

  memcopy(written,
          (uint32_t *)(((uintptr_t)tfs->buffer_bat) +
                       (offset % TFS_BLOCK_SIZE)),
          buffer);

  req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[b1]);
  req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem   = NULL;
  r = tfs->disk->write_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  buffer = (void *)((uintptr_t)buffer + written);
  b1++;
  while(b1 <= b2) {

    if(b1 == b2) {
      /* Last block. If partial write, read the block first.
         Write anyway always to the beginning of the block */
      if((datasize - written)  < TFS_BLOCK_SIZE) {
        req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[b1]);
        req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
        req.sem   = NULL;
        r = tfs->disk->read_block(tfs->disk, &req);
        if(r == 0) {
          /* An error occured. */
          semaphore_V(tfs->lock);
          return VFS_ERROR;
        }
      }

      memcopy(datasize - written,
              (uint32_t *)(uintptr_t)tfs->buffer_bat,
              buffer);
      written = datasize;
    }
    else {
      /* Write whole block */
      memcopy(TFS_BLOCK_SIZE,
              (uint32_t *)(uintptr_t)tfs->buffer_bat,
              buffer);
      written += TFS_BLOCK_SIZE;
      buffer = (void *)((uintptr_t)buffer + TFS_BLOCK_SIZE);
    }

    req.block = tfs->startblock + from_big_endian32(tfs->buffer_inode->block[b1]);
    req.buf   = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
    req.sem   = NULL;
    r = tfs->disk->write_block(tfs->disk, &req);
    if(r == 0) {
      /* An error occured. */
      semaphore_V(tfs->lock);
      return VFS_ERROR;
    }

    b1++;
  }

  semaphore_V(tfs->lock);
  return written;
}

/**
 * Get number of free bytes on the disk. Implements fs.getfree().
 * Reads allocation blocks and counts number of zeros in the bitmap.
 * Result is multiplied by the block size and returned.
 *
 * @param fs Pointer to the fs data structure of the device.
 *
 * @return Number of free bytes.
 */
int tfs_getfree(fs_t *fs)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  gbd_request_t req;
  int allocated = 0;
  uint32_t i;
  int r;

  semaphore_P(tfs->lock);

  req.block = tfs->startblock + TFS_ALLOCATION_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_bat);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    /* An error occured. */
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  for(i=0;i<tfs->totalblocks;i++) {
    allocated += bitmap_get(tfs->buffer_bat,i);
  }

  semaphore_V(tfs->lock);
  return (tfs->totalblocks - allocated)*TFS_BLOCK_SIZE;
}

/* Get the count of files in the directory if it exists (ie. only the
 * master directory is accepted. */
int tfs_filecount(fs_t *fs, char *dirname)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  gbd_request_t req;
  uint32_t i;
  int r;
  int count = 0;

  if (stringcmp(dirname, "/") != 0)
    return VFS_NOT_FOUND;

  semaphore_P(tfs->lock);

  req.block = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);
  if(r == 0) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  for(i=0; i < TFS_MAX_FILES; ++i) {
    if(tfs->buffer_md[i].inode != 0) {
      ++count;
    }
  }

  semaphore_V(tfs->lock);
  return count;
}

/* Get the name of the file with index idx in the directory dirname.
 * There is only one directory in flatfs, so we check that dirname == "".
 * This function is ineffective and we could improve it by guaranteeing
 * the file indexes used are the n first. */
int tfs_file(fs_t *fs, char *dirname, int idx, char *buffer)
{
  tfs_t *tfs = (tfs_t *)fs->internal;
  uint32_t i;
  int r;
  int count = 0;
  gbd_request_t req;

  if (stringcmp(dirname, "/") != 0 || idx < 0)
    return VFS_ERROR;

  semaphore_P(tfs->lock);

  req.block = tfs->startblock + TFS_DIRECTORY_BLOCK;
  req.buf = ADDR_KERNEL_TO_PHYS((uintptr_t)tfs->buffer_md);
  req.sem = NULL;
  r = tfs->disk->read_block(tfs->disk, &req);

 if(r == 0) {
    semaphore_V(tfs->lock);
    return VFS_ERROR;
  }

  for(i=0; i < TFS_MAX_FILES; ++i)
    {
      if(tfs->buffer_md[i].inode != 0 && count++ == idx)
        {
          stringcopy(buffer, tfs->buffer_md[i].name,
                     TFS_FILENAME_MAX);
          semaphore_V(tfs->lock);
          return VFS_OK;
        }
    }

  semaphore_V(tfs->lock);
  return VFS_ERROR;
}

/** @} */
