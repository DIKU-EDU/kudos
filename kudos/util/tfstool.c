/*
 * TFS tool handling.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define TYPES_H          1
#define KUDOS_LIB_LIBC_H 1

#include "fs/tfs_constants.h"
#include "lib/bitmap.h"
#include "util/tfstool.h"

void tfstool_createvol(char *diskname, int size, char *volname);
void tfstool_list(char *filename);
void tfstool_write(char *diskname, char *source, char *target);
unsigned long getfilesize(FILE *fp);
long tfstool_numblocks(FILE *disk);
void tfstool_delete(char *diskname, char *filename);
void tfstool_read(char *diskname, char *source, char *target);
FILE *openfile(char *filename, const char *mode);
void read_block(block_t data, int block);
void write_block(block_t data, int block);

FILE *disk;

void print_usage(void)
{
  printf("KUDOS Trivial Filesystem (TFS) Tool -- Version %s\n\n",
         TFSTOOL_VERSION);

  printf("Copyright (C) 2003-2016  Juha Aatrokoski, Timo Lilja,\n");
  printf("  Leena Salmela, Teemu Takanen, Aleksi Virtanen, Philip Meulengracht,\n");
  printf("  Troels Henriksen, Annie Jane Pinder, Niels Gustav Westphal Serup,\n");
  printf("  Nicklas Warming Jacobsen, Oleksandr Shturmov.\n");
  printf("See the file COPYING for licensing details.\n");
  printf("\n");

  printf("Usage: tfstool arguments ...\n");
  printf("Commands:\n");
  printf("  create <image name> <size in %d-byte blocks> <volume name>\n",
         TFS_BLOCK_SIZE);
  printf("  list   <image name>\n");
  printf("  write  <image name> <local file name> [<tfs filename>]\n");
  printf("  read   <image name> <TFS filename> [<local filename>]\n");
  printf("  delete <image name> <TFS filename>\n");
  printf("\n");
  printf("N.B.: You need to make the size at least 3 blocks in order to\n");
  printf("      include header, allocaton table and master directory.\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  char diskfilename[FILENAME_MAX];
  char localfilename[FILENAME_MAX];
  char tfsfilename[TFS_FILENAME_MAX];
  char volname[TFS_VOLNAME_MAX];
  size_t size;


  if (argc < 3)
    print_usage();

  if (!strncmp(argv[1], "create", 6)) {
    if (argc != 5)
      print_usage();

    strncpy(diskfilename, argv[2], FILENAME_MAX);
    size = (size_t)strtoul(argv[3], NULL, 10);
    strncpy(volname, argv[4], TFS_VOLNAME_MAX);
    volname[TFS_FILENAME_MAX - 1] = '\0';

    tfstool_createvol(diskfilename, size, volname);
  } else if (!strncmp(argv[1], "list", 4)) {
    if (argc != 3)
      print_usage();

    strncpy(diskfilename, argv[2], FILENAME_MAX);

    tfstool_list(diskfilename);
  } else if (!strncmp(argv[1], "write", 5)) {
    if (argc < 4 || argc > 5)
      print_usage();

    strncpy(diskfilename, argv[2], FILENAME_MAX);
    strncpy(localfilename, argv[3], FILENAME_MAX);

    if (argc == 5)
      strncpy(tfsfilename, argv[4], TFS_FILENAME_MAX);
    else
      strncpy(tfsfilename, localfilename, TFS_FILENAME_MAX);
    tfsfilename[TFS_FILENAME_MAX - 1] = '\0';

    tfstool_write(diskfilename, localfilename, tfsfilename);
  } else if (!strncmp(argv[1], "read", 4)) {
    if (argc < 4 || argc > 5)
      print_usage();

    strncpy(diskfilename, argv[2], FILENAME_MAX);
    strncpy(tfsfilename, argv[3], TFS_FILENAME_MAX);


    if (argc == 5)
      strncpy(localfilename, argv[4], FILENAME_MAX);
    else
      strncpy(localfilename, tfsfilename, FILENAME_MAX);

    tfstool_read(diskfilename, tfsfilename, localfilename);
  } else if (!strncmp(argv[1], "delete", 6)) {
    if (argc != 4)
      print_usage();
    strncpy(diskfilename, argv[2], FILENAME_MAX);
    strncpy(tfsfilename, argv[3], TFS_FILENAME_MAX);

    tfstool_delete(diskfilename, tfsfilename);
  } else {
    print_usage();
  }

  return 0;
}

/* Creates a disk volume named 'diskname', the size of the disk is
   'size' blocks (a block is 512 bytes). */
void tfstool_createvol(char *diskfilename, int size, char *volname)
{
  int i;

  uint32_t tfsmagic = htonl(TFS_MAGIC);
  block_t header, bat;
  /* The size of the allocation bitmap is one block in the filesystem.
     We reserve an array of bitmap_t's totaling TFS_BLOCK_SIZE
     from the stack here.
  */
  bitmap_t allocation[TFS_BLOCK_SIZE/sizeof(bitmap_t)];

  bitmap_init(allocation, 8*TFS_BLOCK_SIZE);
  disk = fopen(diskfilename, "r");
  if (disk != NULL) {
    printf("tfstool: File '%s' already exists?\n", diskfilename);
    exit(EXIT_FAILURE);
  }

  /* check that there is room for all headers in the disk */
  if(size < 3) {
    printf("tfstool: Disk size too small. Disk size must be");
    printf(" at least 3 blocks.\n");
    exit(EXIT_FAILURE);
  }

  disk = openfile(diskfilename, "wb");

  /* zero the header and block allocation table (bat) blocks */
  memset(header, 0, TFS_BLOCK_SIZE);
  memset(bat, 0, TFS_BLOCK_SIZE);

  /* set up the header block and write it */
  memcpy(header, &tfsmagic, 4);
  memcpy(&header[4], volname, TFS_VOLNAME_MAX);
  write_block(header, TFS_HEADER_BLOCK);

  /* set up the block allocation table block and write it */
  bitmap_set(allocation, TFS_HEADER_BLOCK, 1);
  bitmap_set(allocation, TFS_ALLOCATION_BLOCK, 1);
  bitmap_set(allocation, TFS_DIRECTORY_BLOCK, 1);
  memcpy(bat, allocation, TFS_BLOCK_SIZE);
  write_block(bat, TFS_ALLOCATION_BLOCK);

  /* write zero directory block (the disk is initially empty) */
  write_block(NULL, TFS_DIRECTORY_BLOCK);

  /* Write data blocks. initially empty. Start writing from
     first data block (blocik num 3). */
  for (i = 3; i < size; i++)
    write_block(NULL, i);

  fclose(disk);

  printf("Disk image '%s', volume name '%s', size %d blocks created.\n",
         diskfilename, volname, size);
}

/* Copy a file 'source' from host file system to kudos tfs filesystem
   as 'target'. */
void tfstool_write(char *diskfilename, char *source, char *target) {
  block_t allocation_block, master_dir, inode_block, data;
  bitmap_t *bat;
  tfs_direntry_t *direntry;
  tfs_inode_t *inode;
  unsigned int i, num_blocks, bnum, inode_bnum;
  signed int index;
  int writeok = 1;
  uint32_t filesize;

  /* Pointer to source file in host file system. */
  FILE *source_fp;
  unsigned long source_filesize;

  disk = openfile(diskfilename, "r+");

  source_fp = openfile(source, "r");
  source_filesize = getfilesize(source_fp);

  memset(inode_block, 0, TFS_BLOCK_SIZE);

  /* Read master directory from tfs disk file and find free entry.
     If there is no free entries or filename already exists exit
     with error. */
  read_block(master_dir, TFS_DIRECTORY_BLOCK);
  direntry = (tfs_direntry_t *)master_dir;
  index = -1;
  for(i=0; i < TFS_MAX_FILES; i++) {
    if(direntry[i].inode == 0 && index == -1) {
      index = i;
    }
    if(strncmp(direntry[i].name, target, TFS_FILENAME_MAX) == 0) {
      printf("File %s already exists in TFS.\n", target);
      exit(EXIT_FAILURE);
    }
  }
  if(index < 0) {
    printf("TFS full.\n");
    exit(EXIT_FAILURE);
  }

  /* Read allocation block. Find free block from allocation bitmap and
     set it reserved (findnset()).Write block from source file to
     correponding block in tfs file.

     If there is not enough free blocks for file set writeok to zero.
     Allocation block is then not updated (written back to file) so
     it soesn't matter that we have written file blocks to disk. */

  read_block(allocation_block, TFS_ALLOCATION_BLOCK);
  num_blocks = tfstool_numblocks(disk);

  bat = (bitmap_t *)allocation_block;
  inode = (tfs_inode_t *)inode_block;

  inode_bnum = bitmap_findnset(bat, num_blocks);
  if(inode_bnum > 2 && inode_bnum < num_blocks) {
    bnum = 0;
    filesize = 0;
    for(i=0;i<num_blocks && filesize < TFS_MAX_FILESIZE;i++) {
      bnum = bitmap_findnset(bat, num_blocks);
      if(bnum > 2 && bnum < num_blocks) {
        filesize += fread(data, 1, TFS_BLOCK_SIZE, source_fp);
        write_block(data, bnum);
        inode->block[i] = htonl(bnum);

        if(feof(source_fp))
          break;
      } else {
        /* No free block was found */
        writeok = 0;
        break;
      }
    }

    if (filesize != source_filesize) {
      printf("Error: Only %d bytes (of %ld bytes) fit to the file"
             " -- wrote nothing.\n", filesize, source_filesize);
      fclose(source_fp);
      fclose(disk);
      exit(EXIT_FAILURE);
    }

    if(writeok) {
      /* Write allocation block and inode block. Inode must be
         written here because we don't earlier know the file size. */
      inode->filesize = htonl(filesize);
      write_block(inode_block, inode_bnum);

      direntry[index].inode = htonl(inode_bnum);
      strncpy(direntry[index].name, target, TFS_FILENAME_MAX);
      write_block(master_dir, TFS_DIRECTORY_BLOCK);

      write_block(allocation_block, TFS_ALLOCATION_BLOCK);
    } else {
      printf("Error: while writing file to tfs-file (disk full?)\n");
      fclose(source_fp);
      fclose(disk);
      exit(EXIT_FAILURE);
    }
  } else {
    printf("Error: Could not allocate inode (disk full?).\n");
    fclose(source_fp);
    fclose(disk);
    exit(EXIT_FAILURE);
  }

  fclose(source_fp);
  fclose(disk);

  printf("File '%s' written to '%s' as '%s'.\n",
         source, diskfilename, target);
}

/* Copy a file 'source' from kudos tfs filesystem to host filesystem
   as 'target'. */
void tfstool_read(char *diskfilename, char *source, char *target) {
  block_t master_dir;
  block_t inode_block;
  block_t data;
  tfs_direntry_t *direntry;
  tfs_inode_t *inode;
  unsigned int i, bnum, size, filesize;
  signed int index;
  int count = 0;

  /* target file on host file system */
  FILE *t;

  disk = openfile(diskfilename, "r+");
  t = openfile(target, "w");



  /* Read directory block of tfs and find the file to be copied. */
  read_block(master_dir, TFS_DIRECTORY_BLOCK);
  direntry = (tfs_direntry_t *)master_dir;
  index = -1;
  for(i=0; i < TFS_MAX_FILES; i++) {
    if(strncmp(direntry[i].name, source, TFS_FILENAME_MAX) == 0) {
      index = i;
      break;
    }
  }
  if(index < 0) {
    printf("File '%s' not found.\n", source);
    exit(EXIT_FAILURE);
  }

  /* Read inode block of the file. */
  read_block(inode_block, ntohl(direntry[index].inode));
  inode = (tfs_inode_t *)inode_block;

  /* Get file blocks from inode. read corresponding blocks from tfs
     and write them to host file system. */
  filesize = ntohl(inode->filesize);
  for (i = 0; i < (int) (TFS_BLOCKS_MAX) &&
         (bnum = ntohl(inode->block[i])) != 0; i++) {
    read_block(data, bnum);

    /* If there is less than block size to write, write only that.
       Rest of the block doesn't belong and is not wanted to
       the file. */
    size = filesize - count;
    if(size > TFS_BLOCK_SIZE)
      size = TFS_BLOCK_SIZE;

    count += fwrite(data, 1, size, t);
  }

  printf("%d bytes written to file '%s'.\n", count, target);

  fclose(t);
  fclose(disk);

}

/* Lists the files in the image file named 'diskfilename'. */
void tfstool_list(char *diskfilename) {
  block_t header, master_dir;
  tfs_direntry_t *direntry;
  unsigned int i;
  int numblocks;

  disk = openfile(diskfilename, "r");

  read_block(header, TFS_HEADER_BLOCK);
  read_block(master_dir, TFS_DIRECTORY_BLOCK);
  direntry = (tfs_direntry_t *)master_dir;
  numblocks = tfstool_numblocks(disk);

  printf("diskfilename: %s, volume name: %s, volume blocks: %d\n\n",
         diskfilename, (char *)(header + 4), numblocks);

  printf("inode  size  name               block numbers\n");
  for (i = 0; i < TFS_MAX_FILES; i++) {
    if (direntry[i].inode > 0) {
      block_t data;
      tfs_inode_t *inode;
      int j;

      /* We need to fetch each file's inode to get filesize. */
      read_block(data, ntohl(direntry[i].inode));
      inode = (tfs_inode_t *)data;

      printf("  %3u %5u  %-16s", 
             (unsigned int) ntohl(direntry[i].inode),
             (unsigned int) ntohl(inode->filesize),
             direntry[i].name);
      for (j = 0; j < (int) (TFS_BLOCKS_MAX) && ntohl(inode->block[j]) != 0; j++)
        printf(" %3u", (unsigned int) ntohl(inode->block[j]));
      printf("\n");
    }
  }
}

/* Deletes file 'filename' from the disk 'diskfilename'. */
void tfstool_delete(char *diskfilename, char *filename)
{
  block_t allocation_block;
  block_t master_dir;
  block_t inode_block;
  bitmap_t *bat;
  tfs_direntry_t *direntry;
  tfs_inode_t *inode;
  signed int index;
  unsigned int i;

  memset(allocation_block, 0, TFS_BLOCK_SIZE);
  memset(master_dir, 0, TFS_BLOCK_SIZE);
  memset(inode_block, 0, TFS_BLOCK_SIZE);

  disk = openfile(diskfilename, "r+");

  /* We read the master_dir and find the inode of the file
   * named 'filename'. */
  read_block(master_dir, TFS_DIRECTORY_BLOCK);
  direntry = (tfs_direntry_t *)master_dir;
  index = -1;
  for(i=0; i < TFS_MAX_FILES; i++) {
    if(strncmp(direntry[i].name, filename, TFS_FILENAME_MAX) == 0) {
      index = i;
      break;
    }
  }

  if(index < 0) {
    printf("File '%s' not found.\n", filename);
    exit(EXIT_FAILURE);
  } else {
    uint32_t inode_bn;
    read_block(allocation_block, TFS_ALLOCATION_BLOCK);
    bat = (bitmap_t *)allocation_block;

    inode_bn = ntohl(direntry[index].inode);
    read_block(inode_block, inode_bn);
    inode = (tfs_inode_t *)inode_block;

    /* Release the blocks reserved for the file. */
    for (i = 0; i < (int) (TFS_BLOCKS_MAX) && ntohl(inode->block[i]) != 0; i++)
      bitmap_set(bat, ntohl(inode->block[i]), 0);

    /* Release the inode block */
    bitmap_set(bat, inode_bn, 0);

    /* Empty entry in TFS is one with NULL as it's direntry's
       inode and name[0]. */
    direntry[index].inode   = 0;
    direntry[index].name[0] = '\0';

    write_block(allocation_block, TFS_ALLOCATION_BLOCK);
    write_block(master_dir, TFS_DIRECTORY_BLOCK);
  }
  fclose(disk);

  printf("File '%s' deleted from '%s'.\n", filename, diskfilename);
}

unsigned long getfilesize(FILE *fp)
{
  long size, pos;

  if ((pos = ftell(fp)) == -1)
    perror("ftell");

  if (fseek(fp, 0, SEEK_END) != 0)
    perror("fseek");

  if ((size = ftell(fp)) == -1)
    perror("ftell");

  if (fseek(fp, pos, SEEK_SET) != 0)
    perror("fseek");

  return (unsigned long)size;
}

/* Return total number of blocks of the disk. */
long tfstool_numblocks(FILE *disk)
{
  return (getfilesize(disk) / TFS_BLOCK_SIZE);
}


FILE *openfile(char *filename, const char *mode)
{
  FILE *fp;
  fp = fopen(filename, mode);

  if (fp == NULL) {
    printf("Unable to open file: %s\n", filename);
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  return fp;
}

/* Read 'block' of tfs file to 'data'. */
void read_block(block_t data, int block)
{
  size_t __attribute__ ((unused)) ret;

  if (fseek(disk, block*TFS_BLOCK_SIZE, SEEK_SET) != 0) {
    perror("read_block:fseek");
    exit(EXIT_FAILURE);
  }

  ret = fread(data, TFS_BLOCK_SIZE, 1, disk);

  if (ferror(disk)) {
    printf("error reading block: %d\n", block);
    exit(EXIT_FAILURE);
  }
}

/* Write 'data' to tfs block 'block' */
void write_block(block_t data, int block)
{
  block_t nullblock;

  if (fseek(disk, block*TFS_BLOCK_SIZE, SEEK_SET) != 0) {
    perror("fseek");
    exit(EXIT_FAILURE);
  }

  if (data == NULL) {
    memset(nullblock, 0, TFS_BLOCK_SIZE);
    data = nullblock;
  }
  if (fwrite(data, 1, TFS_BLOCK_SIZE, disk) == 0 && ferror(disk)) {
    perror("fwrite");
    exit(EXIT_FAILURE);
  }
}


/* bitmap routines taken from kudos/bitmap.c */

/**
 * Calculates the memory size in bytes needed to store a given number
 * of bits in a bitmap. The size of the bitmap will be a multiple of
 * 4.
 *
 * @param num_bits The number of bits the bitmap needs to hold.
 *
 * @return The size of the bitmap in bytes.
 */
int bitmap_sizeof(int num_bits)
{
  return ((num_bits + 31) / 32) * 4;
}

/**
 * Initialize a given bitmap. All entries in the bitmap are intialized
 * to 0.
 *
 * @param bitmap The bitmap to initialize
 *
 * @param size The number of bits in the bitmap.
 */
void bitmap_init(bitmap_t *bitmap, int size)
{
  int i;
  for(i = 0; i < (size + 31) / 32; i++)
    bitmap[i] = 0;
}

/**
 * Gets the value of a given bit in the bitmap.
 *
 * @param bitmap The bitmap
 *
 * @param pos The position of the bit, whose value will be returned.
 *
 * @return The value (0 or 1) of the given bit in the bitmap.
 */
int bitmap_get(bitmap_t *bitmap, int pos)
{
  int i;
  int j;

  i = pos / 32;
  j = pos % 32;

  return ((ntohl(bitmap[i]) >> j) & 0x01);
}

/**
 * Sets the given bit in the bitmap.
 *
 * @param bitmap The bitmap
 *
 * @param pos The index of the bit to set
 *
 * @param value The new value of the given bit. Valid values are 0 and
 * 1.
 */
void bitmap_set(bitmap_t *bitmap, int pos, int value)
{
  int i;
  int j;

  i = pos / 32;
  j = pos % 32;

  if (value == 0) {
    bitmap[i] = htonl(ntohl(bitmap[i]) & ~(1 << j));
  } else if (value == 1) {
    bitmap[i] = htonl(ntohl(bitmap[i]) | (1 << j));
  } else {
    printf("bitmap_set:bit value other than 0 or 1");
    exit(EXIT_FAILURE);
  }
}


/**
 * Finds first zero and sets it to one.
 *
 * @param bitmap The bitmap
 *
 * @param l Length of bitmap in bits
 *
 * @return Number of bit set. Negative if failed.
 */
int bitmap_findnset(bitmap_t *bitmap, int l)
{
  int i;

  for(i=0;i<l;i++) {
    if(bitmap_get(bitmap,i) == 0) {
      bitmap_set(bitmap, i, 1);
      break;
    }
  }

  if(i >= l)
    i = -1;

  return i;
}


