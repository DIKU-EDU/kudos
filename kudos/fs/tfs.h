/*
 * Trivial Filesystem (TFS).
 */

#ifndef KUDOS_FS_TFS_H
#define KUDOS_FS_TFS_H

#include "fs/tfs_constants.h"

#include "drivers/gbd.h"
#include "fs/vfs.h"
#include "lib/libc.h"
#include "lib/bitmap.h"

/* functions */
fs_t * tfs_init(gbd_t *disk, uint32_t sector);

int tfs_unmount(fs_t *fs);
int tfs_open(fs_t *fs, char *filename);
int tfs_close(fs_t *fs, int fileid);
int tfs_create(fs_t *fs, char *filename, int size);
int tfs_remove(fs_t *fs, char *filename);
int tfs_read(fs_t *fs, int fileid, void *buffer, int bufsize, int offset);
int tfs_write(fs_t *fs, int fileid, void *buffer, int datasize, int offset);
int tfs_getfree(fs_t *fs);
int tfs_filecount(fs_t *fs, char *dirname);
int tfs_file(fs_t *fs, char *dirname, int idx, char *buffer);

#endif // KUDOS_FS_TFS_H
