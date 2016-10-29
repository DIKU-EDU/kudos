/*
 * List of available filesystem drivers.
 */

#ifndef KUDOS_FS_FILESYSTEMS_H
#define KUDOS_FS_FILESYSTEMS_H

#include "fs/vfs.h"
#include "drivers/gbd.h"

typedef struct filesystems_struct_t {
    /* Name of the filesystem driver. */
    const char *name;

    /* Function pointer to a function which tries to mount filesystem
       on given disk. If mount fails, NULL is returned and disk is
       unmodified. The returned structure must be later deallocated by
       calling unmount-function (defined as function pointer in
       fs_t). */
    fs_t *(*init)(gbd_t *disk, uint32_t sector);

} filesystems_t;

fs_t *filesystems_try_all(gbd_t *disk);

#endif // KUDOS_FS_FILESYSTEMS_H
