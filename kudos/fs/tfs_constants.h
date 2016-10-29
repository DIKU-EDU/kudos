/*
 * Trivial Filesystem (TFS).
 */

/*
 * This file defines TFS constants that can also be safely imported by
 * host (Linux) userspace - specifically, the tfstool program.  This
 * is a special case for TFS and should not be done by other file
 * system drivers.
 */

#ifndef KUDOS_FS_TFS_CONSTANTS_H
#define KUDOS_FS_TFS_CONSTANTS_H

/* In TFS block size is 512. This will affect to various other
   features of TFS e.g. maximum file size. */
#define TFS_BLOCK_SIZE 512

/* Magic number found on each tfs filesystem's header block. */
#define TFS_MAGIC 3745

/* Block numbers for system blocks */
/* Modify the base block to suit where the TFS partition is located */
#define TFS_HEADER_BLOCK 0
#define TFS_ALLOCATION_BLOCK 1
#define TFS_DIRECTORY_BLOCK  2

/* Names are limited to 16 characters */
#define TFS_VOLNAME_MAX 16
#define TFS_FILENAME_MAX 16

/*
  Maximum number of block pointers in one inode. Block pointers
  are of type uint32_t and one pointer "slot" is reserved for
  file size.
*/
#define TFS_BLOCKS_MAX ((TFS_BLOCK_SIZE/sizeof(uint32_t))-1)

/* Maximum file size. 512-byte Inode can store 127 blocks for a file.
   512*127=65024 */
#define TFS_MAX_FILESIZE (TFS_BLOCK_SIZE*TFS_BLOCKS_MAX)

/* File inode block. Inode contains the filesize and a table of blocknumbers
   allocated for the file. In TFS files can't have more blocks than fits in
   block table of the inode block.

   One 512 byte block can hold 128 32-bit integers. Therefore the table
   size is limited to 127 and filesize to 127*512=65024.
*/

typedef struct {
  /* filesize in bytes */
  uint32_t filesize;

  /* block numbers allocated for this file, zero
     means unused block. */
  uint32_t block[TFS_BLOCKS_MAX];
} tfs_inode_t;


/* Master directory block entry. If inode is zero, entry is
   unused (free). */
typedef struct {
  /* File's inode block number. */
  uint32_t inode;

  /* File name */
  char     name[TFS_FILENAME_MAX];
} tfs_direntry_t;

#define TFS_MAX_FILES (TFS_BLOCK_SIZE/sizeof(tfs_direntry_t))

#endif // KUDOS_FS_TFS_CONSTANTS_H
