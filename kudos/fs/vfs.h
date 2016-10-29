/*
 * Virtual Filesystem (VFS).
 */

#ifndef KUDOS_FS_VFS_H
#define KUDOS_FS_VFS_H

#include "drivers/gbd.h"

/* Return codes for filesytem functions. Positive return values equal
   to VFS_OK */
#define VFS_OK              (0)
#define VFS_NOT_SUPPORTED   (-1)
#define VFS_ERROR           (-2)
#define VFS_INVALID_PARAMS  (-3)
#define VFS_NOT_OPEN        (-4)
#define VFS_NOT_FOUND       (-5)
#define VFS_NO_SUCH_FS      (-6)
#define VFS_LIMIT           (-7)
#define VFS_IN_USE          (-8)
#define VFS_UNUSABLE        (-9)

/* Maximum length of filename without mountpoint and directory names. */
#define VFS_NAME_LENGTH 16

/* Maximum length of the full name of a file, including mountpoint and
   directories. */
#define VFS_PATH_LENGTH 256

/* Type for open file entries. This is actually index to open files table */
typedef int openfile_t;

/* Structure defining a filesystem driver instance for one filesystem.
   Instances of this structure are created by filesystem init-function and
   they are used only inside VFS. */
typedef struct fs_struct{
  /* Driver internal data */
  void *internal;
  /* Name of the volume (advisory mounpoint) */
  char volume_name[16];

  /* Function pointer to a function which will be called when this
     filesystem is unmounted. In this call the driver must flush
     all cached data to disk and release allocated structures,
     including the filesystem structure given as argument. */
  int (*unmount)(struct fs_struct *fs);

  /* Function pointer to a function which opens a file in the
     filesystem. A pointer to this structure as well as name of a
     file is given as argument. Returns non-negative file id which
     must be unique for this filesystem. Negative values are errors. */
  int (*open)(struct fs_struct *fs, char *filename);

  /* Function pointer to a function which closes the given (open) file.
     A pointer to this structure as will as fileid previously returned
     by open is given as argument. Zero return value is success,
     negative values are errors. */
  int (*close)(struct fs_struct *fs, int fileid);

  /* Function pointer to a function which reads at most bufsize
     bytes from given open file (fileid). A pointer to this
     structure is given as first argument. The data is read to given
     buffer and read is started from given absolute (not relative to
     seek position) offset in the file.

     The number of bytes actually read is returned. Zero is returned
     only at the end of the file (nothing to read). Negative values
     are errors. */
  int (*read)(struct fs_struct *fs, int fileid, void *buffer,
              int bufsize, int offset);

  /* Function pointer to a function which writes datasize
     bytes to given open file (fileid). A pointer to this
     structure is given as first argument. The data is read from given
     buffer and write is started from given absolute (not relative to
     seek position) offset in the file.

     The number of bytes actually written is returned. Any value
     other than datasize as return code is error.
  */
  int (*write)(struct fs_struct *fs, int fileid, void *buffer,
               int datasize, int offset);

  /* Function pointer to a function which creates new file in the
     filesystem. A pointer to this structure is given as the first
     argument, name of the file to be created as second argument and
     size of the created file as third.

     Returns success value as defined above (VFS_OK, etc.) */
  int (*create)(struct fs_struct *fs, char *filename, int size);

  /* Function pointer to a function which removes file from the
     filesystem. A pointer to this structure is given as the first
     argument and name of the file to be deleted as the second argument.

     Returns success value as defined above (VFS_OK, etc.) */
  int (*remove)(struct fs_struct *fs, char *filename);

  /* Function pointer to a function which returns the number of free
     bytes in the filesystem. Pointer to this structure is given as
     argument to the function.

     Returns the number of free bytes, negative values are errors. */
  int (*getfree)(struct fs_struct *fs);

  /* Function pointer to a function which returns the count of files
     in a directory, or the number of file systems if the name is
     NULL.

     Returns success value as defined above (VFS_OK, etc.)
  */
  int (*filecount)(struct fs_struct *fs, char *dirname);

  /* Function pointer to a function which reads the name of a file
     into the specified buffer. The file is in the directory and has
     index idx.

     Returns success value as defined above (VFS_OK, etc.)
  */
  int (*file)(struct fs_struct *fs, char *dirname, int idx, char *buf);
} fs_t;


void vfs_init(void);
void vfs_mount_all(void);
void vfs_deinit(void);

int vfs_mount_fs(gbd_t *disk, char *volumename);
int vfs_mount(fs_t *fs, char *name);
int vfs_unmount(char *name);

openfile_t vfs_open(char *pathname);
int vfs_close(openfile_t file);
int vfs_seek(openfile_t file, int seek_position);
int vfs_read(openfile_t file, void *buffer, int bufsize);
int vfs_write(openfile_t file, void *buffer, int datasize);

int vfs_create(char *pathname, int size);
int vfs_remove(char *pathname);
int vfs_getfree(char *filesystem);

int vfs_filecount(char *pathname);
int vfs_file(char *pathname, int idx, char *buffer);

#endif // KUDOS_FS_VFS_H
