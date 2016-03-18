/*
 * Virtual Filesystem (VFS).
 */

#include "fs/vfs.h"
#include "kernel/semaphore.h"
#include "kernel/assert.h"
#include "kernel/config.h"
#include "lib/libc.h"
#include "drivers/device.h"
#include "fs/tfs.h"
#include "fs/filesystems.h"

/** @name Virtual Filesystem
 *
 *  This module implements one virtual filesystem in which all actual
 *  filesystems are mounted so that they behave like one big filesystem.
 *
 *  @{
 */

/* Mounted filesystem information structure. */
typedef struct {
  /* Pointer to filesystem driver. */
  fs_t *filesystem;

  /* Name of the mountpoint. */
  char mountpoint[VFS_NAME_LENGTH];
} vfs_entry_t;

/* Open file information */
typedef struct {
  /* Filesystem in which this open file is. */
  fs_t *filesystem;

  /* Filesystem specific file id for this open file. */
  int fileid;

  /* Current seek position in the file. */
  int seek_position;
} openfile_entry_t;


/* Table of mounted filesystems. */
static struct {
  /* Binary semaphore for locking this table. */
  semaphore_t *sem;

  /* Table of mounted filesystems. */
  vfs_entry_t filesystems[CONFIG_MAX_FILESYSTEMS];
} vfs_table;


/* Table of open files. */
static struct {
  /* Binary semaphore for locking this table. */
  semaphore_t *sem;

  /* Table of open files. */
  openfile_entry_t files[CONFIG_MAX_OPEN_FILES];
} openfile_table;

/* The following variables are used to synchronize the forced unmount
   used when shutting down the system so that the filesystems are
   clean. */

/* Binary semaphore to synchronize access to vfs_ops and vfs_usable */
static semaphore_t *vfs_op_sem;

/* This semaphore is used to wake up the pending unmount operation
   when VFS is being shut down and all pending operations are
   complete */
static semaphore_t *vfs_unmount_sem;

/* The number of active operations on VFS */
static int vfs_ops;

/* Boolean which indicates whether VFS is currently usable. When VFS
   becomes unusable it will never be usable again because this is used
   when halting the system. */
static int vfs_usable = 0;

/**
 * Initializes Virtual Filesystem layer. This function is called
 * before virtual memory is enabled.
 *
 */

void vfs_init(void)
{
  int i;

  vfs_table.sem = semaphore_create(1);
  openfile_table.sem = semaphore_create(1);

  KERNEL_ASSERT(vfs_table.sem != NULL && openfile_table.sem != NULL);

  /* Clear table of mounted filesystems. */
  for(i=0; i<CONFIG_MAX_FILESYSTEMS; i++) {
    vfs_table.filesystems[i].filesystem = NULL;
  }

  /* Clear table of open files. */
  for (i = 0; i < CONFIG_MAX_OPEN_FILES; i++) {
    openfile_table.files[i].filesystem = NULL;
  }

  vfs_op_sem = semaphore_create(1);
  vfs_unmount_sem = semaphore_create(0);

  vfs_ops = 0;
  vfs_usable = 1;

  kprintf("VFS: Max filesystems: %d, Max open files: %d\n",
          CONFIG_MAX_FILESYSTEMS, CONFIG_MAX_OPEN_FILES);
}

/**
 * Force unmount on all filesystems. This function should be used only
 * when halting the system. Waits for all VFS operations to complete
 * but does not wait for all files to be closed. After this function
 * is called the VFS and the whole operating system can no longer be
 * used.
 */
void vfs_deinit(void)
{
  fs_t *fs;
  int row;

  semaphore_P(vfs_op_sem);
  vfs_usable = 0;

  kprintf("VFS: Entering forceful unmount of all filesystems.\n");
  if (vfs_ops > 0) {
    kprintf("VFS: Delaying force unmount until the pending %d "
            "operations are done.\n", vfs_ops);
    semaphore_V(vfs_op_sem);
    semaphore_P(vfs_unmount_sem);
    semaphore_P(vfs_op_sem);
    KERNEL_ASSERT(vfs_ops == 0);
    kprintf("VFS: Continuing forceful unmount.\n");
  }

  semaphore_P(vfs_table.sem);
  semaphore_P(openfile_table.sem);

  for (row = 0; row < CONFIG_MAX_FILESYSTEMS; row++) {
    fs = vfs_table.filesystems[row].filesystem;
    if (fs != NULL) {
      kprintf("VFS: Forcefully unmounting volume [%s]\n",
              vfs_table.filesystems[row].mountpoint);
      fs->unmount(fs);
      vfs_table.filesystems[row].filesystem = NULL;
    }
  }

  semaphore_V(openfile_table.sem);
  semaphore_V(vfs_table.sem);
  semaphore_V(vfs_op_sem);
}


/**
 * Attempts to mount given disk with given mount-point (volumename).
 * All filesystems defined in filesystems.c are attempted in
 * order and first match is used.
 *
 * @param disk Generic Block Device on which some filesystem should be.
 *
 * @param volumename Mount point which should be used for the
 * filesystem. if volumename is NULL, name defined by filesystem
 * driver is used (stored on disk).
 *
 * @return 0 (VFS_OK) on success, non-zero (VFS_*) on error.
 *
 */

int vfs_mount_fs(gbd_t *disk, char *volumename)
{
  fs_t *filesystem;
  int ret;

  filesystem = filesystems_try_all(disk);
  if(filesystem == NULL) {
    kprintf("VFS: No filesystem was found on block device 0x%8.8x\n",
            disk->device->io_address);
    return VFS_NO_SUCH_FS;
  }

  if(volumename==NULL)
    volumename=filesystem->volume_name;

  if(volumename[0] == '\0') {
    kprintf("VFS: Unknown filesystem volume name,"
            " skipping mounting\n");
    filesystem->unmount(filesystem);
    return VFS_INVALID_PARAMS;
  }

  if((ret=vfs_mount(filesystem, volumename)) == VFS_OK) {
    kprintf("VFS: Mounted filesystem volume [%s]\n",
            volumename);
  } else {
    kprintf("VFS: Mounting of volume [%s] failed\n",
            volumename);
    filesystem->unmount(filesystem);
  }

  return ret;
}

/**
 * Mounts all filesystems found in all disks of the system.
 * Tries all known filesystems for all disks.
 *
 */

void vfs_mount_all(void)
{
  int i;
  device_t *dev;

  for(i=0; i<CONFIG_MAX_FILESYSTEMS; i++) {
    dev = device_get(TYPECODE_DISK, i);
    if(dev == NULL) {
      /* No more disks. */
      return;
    } else {
      gbd_t *gbd;
      gbd = (gbd_t *) dev->generic_device;

      if(gbd == NULL) {
        kprintf("VFS: Warning, invalid disk driver detected, "
                "skipping\n");
        continue;
      }

      vfs_mount_fs(gbd, NULL);
    }
  }

}

/**
 * Get pointer to mounted filesystem based on mountpoint name. Note
 * that mount table must be locked before this function is called to be
 * sure that the returned information is valid.
 *
 * @param mountpoint Name of mountpoint
 *
 * @return Pointer to filesystem, NULL if filesystem is not mounted.
 *
 */

static fs_t *vfs_get_filesystem(char *mountpoint)
{
  int row;

  for (row = 0; row < CONFIG_MAX_FILESYSTEMS; row++) {
    if(!stringcmp(vfs_table.filesystems[row].mountpoint, mountpoint)) {
      return vfs_table.filesystems[row].filesystem;
    }
  }

  return NULL;
}

/**
 * Parse pathname into volume (mountpoint) and filename parts.
 *
 * @param pathname Full pathname to parse
 *
 * @param volumebuf Buffer of at least VFS_NAME_LENGTH bytes long
 * where the volume name will be stored.
 *
 * @param filenamebuf Buffer of at least VFS_NAME_LENGTH bytes long
 * where the file name will be stored.
 *
 * @return VFS_ERROR or VFS_OK. On VFS_ERROR the volumebuf and
 * filenamebuf have unspecified contents.
 *
 */

static int vfs_parse_pathname(char *pathname,
                              char *volumebuf,
                              char *filenamebuf)
{
  int i;

  if (pathname[0] == '[') {
    pathname++;
    for(i = 0; i < VFS_NAME_LENGTH - 1; i++) {
      if (*pathname == '\0') {
        return VFS_ERROR;
      }
      if (*pathname == ']') {
        pathname++;
        break;
      }

      *volumebuf = *pathname;
      volumebuf++;
      pathname++;
    }
    if (i >= VFS_NAME_LENGTH - 1)
      return VFS_ERROR;
  }
  *volumebuf = '\0';

  for(i = 0; i < VFS_NAME_LENGTH; i++) {
    *filenamebuf = *pathname;
    if (*pathname == '\0') {
      /* Empty filenames are not allowed. */
      if(i == 0)
        return VFS_ERROR;

      return VFS_OK;
    }
    pathname++;
    filenamebuf++;
  }

  return VFS_ERROR;
}

/**
 * Start a new operation on VFS. Operation is defined to be any such
 * sequence of actions (a VFS function call) that may touch some
 * filesystem.
 *
 * @return VFS_OK if operation can continue, error (negative) if
 * operation must be cancelled.
 */
static int vfs_start_op()
{
  int ret = VFS_OK;

  semaphore_P(vfs_op_sem);

  if (vfs_usable) {
    vfs_ops++;
  } else {
    ret = VFS_UNUSABLE;
  }

  semaphore_V(vfs_op_sem);

  return ret;
}

/**
 * End a VFS operation.
 */
static void vfs_end_op()
{
  semaphore_P(vfs_op_sem);

  vfs_ops--;

  KERNEL_ASSERT(vfs_ops >= 0);

  /* Wake up pending unmount if VFS is now idle. */
  if (!vfs_usable && (vfs_ops == 0))
    semaphore_V(vfs_unmount_sem);

  if (!vfs_usable && (vfs_ops > 0))
    kprintf("VFS: %d operations still pending\n", vfs_ops);

  semaphore_V(vfs_op_sem);
}

/**
 * Mount an initialized filesystem.
 *
 * @param fs Pointer to filesystem driver.
 *
 * @param name Name of the mountpoint where the filesystem should be
 * mounted.
 *
 * @return VFS_LIMIT if too many filesystems are mounted, VFS_ERROR if
 * double mounting is attempted with the same name or VFS_OK if the
 * mount succeeded.
 *
 */

int vfs_mount(fs_t *fs, char *name)
{
  int i;
  int row;

  KERNEL_ASSERT(name != NULL && name[0] != '\0');

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  semaphore_P(vfs_table.sem);

  for (i = 0; i < CONFIG_MAX_FILESYSTEMS; i++) {
    if (vfs_table.filesystems[i].filesystem == NULL)
      break;
  }

  row = i;

  if(row >= CONFIG_MAX_FILESYSTEMS) {
    semaphore_V(vfs_table.sem);
    kprintf("VFS: Warning, maximum mount count exceeded, mount failed.\n");
    vfs_end_op();
    return VFS_LIMIT;
  }

  for (i = 0; i < CONFIG_MAX_FILESYSTEMS; i++) {
    if(stringcmp(vfs_table.filesystems[i].mountpoint, name) == 0) {
      semaphore_V(vfs_table.sem);
      kprintf("VFS: Warning, attempt to mount 2 filesystems "
              "with same name\n");
      vfs_end_op();
      return VFS_ERROR;
    }
  }

  stringcopy(vfs_table.filesystems[row].mountpoint, name, VFS_NAME_LENGTH);
  vfs_table.filesystems[row].filesystem = fs;

  semaphore_V(vfs_table.sem);
  vfs_end_op();
  return VFS_OK;
}

/**
 * Unmounts given filesystem (mountpoint). The filesystem driver's
 * unmount function is called if unmount succeeds.
 *
 * @param name Name of the mountpoint of the filesystem.
 *
 * @return VFS_NOT_FOUND if nothing is mounted to given mountpoint,
 * VFS_IN_USE if the filesystem contains open files and can't be
 * unmounted or VFS_OK if unmounting succeeded.
 *
 */

int vfs_unmount(char *name)
{
  int i, row;
  fs_t *fs = NULL;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  semaphore_P(vfs_table.sem);

  for (row = 0; row < CONFIG_MAX_FILESYSTEMS; row++) {
    if(!stringcmp(vfs_table.filesystems[row].mountpoint, name)) {
      fs = vfs_table.filesystems[row].filesystem;
      break;
    }
  }

  if(fs == NULL) {
    semaphore_V(vfs_table.sem);
    vfs_end_op();
    return VFS_NOT_FOUND;
  }

  semaphore_P(openfile_table.sem);
  for(i = 0; i < CONFIG_MAX_OPEN_FILES; i++) {
    if(openfile_table.files[i].filesystem == fs) {
      semaphore_V(openfile_table.sem);
      semaphore_V(vfs_table.sem);
      vfs_end_op();
      return VFS_IN_USE;
    }
  }

  fs->unmount(fs);
  vfs_table.filesystems[row].filesystem = NULL;

  semaphore_V(openfile_table.sem);
  semaphore_V(vfs_table.sem);
  vfs_end_op();
  return VFS_OK;
}

/**
 * Opens a file on any filesystem.
 *
 * @param pathname Full pathname to file (including mountpoint).
 *
 * @return Open file instance which must be later closed with
 * vfs_close. On error negative value is returned. (VFS_LIMIT,
 * VFS_NOT_FOUND, VFS_NO_SUCH_FS, etc.)
 *
 */

openfile_t vfs_open(char *pathname)
{
  openfile_t file;
  int fileid;
  char volumename[VFS_NAME_LENGTH];
  char filename[VFS_NAME_LENGTH];
  fs_t *fs = NULL;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  if (vfs_parse_pathname(pathname, volumename, filename) != VFS_OK) {
    vfs_end_op();
    return VFS_ERROR;
  }

  semaphore_P(vfs_table.sem);
  semaphore_P(openfile_table.sem);

  for(file=0; file<CONFIG_MAX_OPEN_FILES; file++) {
    if(openfile_table.files[file].filesystem == NULL) {
      break;
    }
  }

  if(file >= CONFIG_MAX_OPEN_FILES) {
    semaphore_V(openfile_table.sem);
    semaphore_V(vfs_table.sem);
    kprintf("VFS: Warning, maximum number of open files exceeded.");
    vfs_end_op();
    return VFS_LIMIT;
  }

  fs = vfs_get_filesystem(volumename);

  if(fs == NULL) {
    semaphore_V(openfile_table.sem);
    semaphore_V(vfs_table.sem);
    vfs_end_op();
    return VFS_NO_SUCH_FS;
  }

  openfile_table.files[file].filesystem = fs;

  semaphore_V(openfile_table.sem);
  semaphore_V(vfs_table.sem);

  fileid = fs->open(fs, filename);

  if(fileid < 0) {
    semaphore_P(openfile_table.sem);
    openfile_table.files[file].filesystem = NULL;
    semaphore_V(openfile_table.sem);
    vfs_end_op();
    return fileid; /* negative -> error*/
  }

  openfile_table.files[file].fileid = fileid;
  openfile_table.files[file].seek_position = 0;

  vfs_end_op();
  return file;
}

/**
 * Verifies that given open file is actually open. Ideally, this should
 * only be called while the open file table lock is held.
 *
 * @param file Openfile id.
 *
 * @return Pointer to openfile table row, or NULl on invalid file.
 */

static openfile_entry_t *vfs_verify_open(openfile_t file)
{
  openfile_entry_t *openfile;

  if (file < 0 && file >= CONFIG_MAX_OPEN_FILES) {
    return NULL;
  }

  openfile = &openfile_table.files[file];

  if (openfile->filesystem == NULL) {
    return NULL;
  }

  return openfile;
}


/**
 * Close open file.
 *
 * @param file Openfile id
 *
 * @return VFS_OK on success, negative (VFS_*) on error.
 *
 */

int vfs_close(openfile_t file)
{
  openfile_entry_t *openfile;
  fs_t *fs;
  int ret;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  semaphore_P(openfile_table.sem);

  openfile = vfs_verify_open(file);
  if (openfile == NULL) {
    semaphore_V(openfile_table.sem);
    return VFS_INVALID_PARAMS;
  }

  fs = openfile->filesystem;

  ret = fs->close(fs, openfile->fileid);
  openfile->filesystem = NULL;

  semaphore_V(openfile_table.sem);

  vfs_end_op();
  return ret;
}


/**
 * Seek given file to given position. The position is not verified
 * to be within the file's size.
 */
int vfs_seek(openfile_t file, int seek_position)
{
  openfile_entry_t *openfile;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  if (seek_position < 0) {
    return VFS_INVALID_PARAMS;
  }

  semaphore_P(openfile_table.sem);

  openfile = vfs_verify_open(file);
  if (openfile == NULL) {
    semaphore_V(openfile_table.sem);
    return VFS_INVALID_PARAMS;
  }

  openfile->seek_position = seek_position;

  semaphore_V(openfile_table.sem);

  vfs_end_op();
  return VFS_OK;
}


/**
 * Reads at most bufsize bytes from given open file to given buffer.
 * The read is started from current seek position and after read, the
 * position is updated.
 *
 * @param file Open file
 *
 * @param buffer Buffer to read from the file
 *
 * @param bufsize maximum number of bytes to read.
 *
 * @return Number of bytes read. Zero indicates end of file and
 * negative values are errors.
 *
 */

int vfs_read(openfile_t file, void *buffer, int bufsize)
{
  openfile_entry_t *openfile;
  fs_t *fs;
  int ret;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  openfile = vfs_verify_open(file);
  if (openfile == NULL) {
    return VFS_INVALID_PARAMS;
  }

  fs = openfile->filesystem;

  KERNEL_ASSERT(bufsize >= 0 && buffer != NULL);

  ret = fs->read(fs, openfile->fileid, buffer, bufsize,
                 openfile->seek_position);

  if(ret > 0) {
    semaphore_P(openfile_table.sem);
    openfile->seek_position += ret;
    semaphore_V(openfile_table.sem);
  }

  vfs_end_op();
  return ret;
}


/**
 * Writes datasize bytes from given buffer to given open file.
 * The write is started from current seek position and after writing, the
 * position is updated.
 *
 * @param file Open file
 *
 * @param buffer Buffer to be written to file.
 *
 * @param datasize Number of bytes to write.
 *
 * @return Number of bytes written. All bytes are written unless error
 * prevented to do that. Negative values are specific error conditions.
 *
 */

int vfs_write(openfile_t file, void *buffer, int datasize)
{
  openfile_entry_t *openfile;
  fs_t *fs;
  int ret;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  openfile = vfs_verify_open(file);
  if (openfile == NULL) {
    return VFS_INVALID_PARAMS;
  }

  fs = openfile->filesystem;

  KERNEL_ASSERT(datasize >= 0 && buffer != NULL);

  ret = fs->write(fs, openfile->fileid, buffer, datasize,
                  openfile->seek_position);

  if(ret > 0) {
    semaphore_P(openfile_table.sem);
    openfile->seek_position += ret;
    semaphore_V(openfile_table.sem);
  }

  vfs_end_op();
  return ret;
}


/**
 * Creates new file.
 *
 * @param pathname Full name of new file, including mountpoint.
 *
 * @param size Initial size of the created file.
 *
 * @param VFS_OK on success, negative (VFS_*) on error.
 *
 */

int vfs_create(char *pathname, int size)
{
  char volumename[VFS_NAME_LENGTH];
  char filename[VFS_NAME_LENGTH];
  fs_t *fs = NULL;
  int ret;

  if (size < 0) {
    return VFS_INVALID_PARAMS;
  }

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  if(vfs_parse_pathname(pathname, volumename, filename) != VFS_OK) {
    vfs_end_op();
    return VFS_ERROR;
  }

  semaphore_P(vfs_table.sem);

  fs = vfs_get_filesystem(volumename);

  if(fs == NULL) {
    semaphore_V(vfs_table.sem);
    vfs_end_op();
    return VFS_NO_SUCH_FS;
  }

  ret = fs->create(fs, filename, size);

  semaphore_V(vfs_table.sem);

  vfs_end_op();
  return ret;
}


/**
 * Removes given file from filesystem.
 *
 * @param pathname Full name of the file, including mountpoint.
 *
 * @return VFS_OK on success, negative (VFS_*) on failure.
 *
 */

int vfs_remove(char *pathname)
{
  char volumename[VFS_NAME_LENGTH];
  char filename[VFS_NAME_LENGTH];
  fs_t *fs = NULL;
  int ret;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  if (vfs_parse_pathname(pathname, volumename, filename) != VFS_OK) {
    vfs_end_op();
    return VFS_ERROR;
  }

  semaphore_P(vfs_table.sem);

  fs = vfs_get_filesystem(volumename);

  if(fs == NULL) {
    semaphore_V(vfs_table.sem);
    vfs_end_op();
    return VFS_NO_SUCH_FS;
  }

  ret = fs->remove(fs, filename);

  semaphore_V(vfs_table.sem);

  vfs_end_op();
  return ret;
}


/**
 * Gets number of free bytes on given filesystem identified by
 * mountpoint-name.
 *
 * @param filesystem Name of mountpoint
 *
 * @return Number of free bytes, negative values (VFS_*) are errors.
 *
 */

int vfs_getfree(char *filesystem)
{
  fs_t *fs = NULL;
  int ret;

  if (vfs_start_op() != VFS_OK)
    return VFS_UNUSABLE;

  semaphore_P(vfs_table.sem);

  fs = vfs_get_filesystem(filesystem);

  if(fs == NULL) {
    semaphore_V(vfs_table.sem);
    vfs_end_op();
    return VFS_NO_SUCH_FS;
  }

  ret = fs->getfree(fs);

  semaphore_V(vfs_table.sem);

  vfs_end_op();
  return ret;
}

int vfs_filecount(char *pathname)
{
    char volumename[VFS_NAME_LENGTH];
    char dirname[VFS_NAME_LENGTH];
    fs_t *fs = NULL;
    int ret;

    if (vfs_start_op() != VFS_OK)
        return VFS_UNUSABLE;

     if (pathname == NULL) {
         semaphore_P(vfs_table.sem);
         for (ret = 0; ret < CONFIG_MAX_FILESYSTEMS; ret++) {
             if (vfs_table.filesystems[ret].filesystem == NULL)
                 break;
         }
         semaphore_V(vfs_table.sem);
         vfs_end_op();
         return ret;
     }

    if (vfs_parse_pathname(pathname, volumename, dirname) != VFS_OK) {
        vfs_end_op();
        return VFS_ERROR;
    }

    semaphore_P(vfs_table.sem);

    fs = vfs_get_filesystem(volumename);

    if(fs == NULL) {
        semaphore_V(vfs_table.sem);
        vfs_end_op();
        return VFS_NO_SUCH_FS;
    }

    ret = fs->filecount(fs, dirname);

    semaphore_V(vfs_table.sem);

    vfs_end_op();
    return ret;
}

int vfs_file(char *pathname, int idx, char *buffer)
{
    char volumename[VFS_NAME_LENGTH];
    char dirname[VFS_NAME_LENGTH];
    fs_t *fs = NULL;
    int ret;

    if (vfs_start_op() != VFS_OK)
        return VFS_UNUSABLE;

    if (pathname == NULL) {
        semaphore_P(vfs_table.sem);
        for (ret = 0; ret < CONFIG_MAX_FILESYSTEMS && idx != 0; ret++) {
            if (vfs_table.filesystems[ret].filesystem != NULL)
                idx--;
        }
        /* Error can be caused if idx was <= 0 or idx was higher than the
         * number of mounted volumes
         */
        if (idx != 0) {
            semaphore_V(vfs_table.sem);
            vfs_end_op();
            return VFS_ERROR;
        }
        stringcopy(buffer, vfs_table.filesystems[ret].mountpoint, VFS_NAME_LENGTH);
        semaphore_V(vfs_table.sem);
        vfs_end_op();
        return VFS_OK;
    }

    if (vfs_parse_pathname(pathname, volumename, dirname) != VFS_OK) {
        vfs_end_op();
        return VFS_ERROR;
    }

    semaphore_P(vfs_table.sem);

    fs = vfs_get_filesystem(volumename);

    if(fs == NULL) {
        semaphore_V(vfs_table.sem);
        vfs_end_op();
        return VFS_NO_SUCH_FS;
    }

    ret = fs->file(fs, dirname, idx, buffer);

    semaphore_V(vfs_table.sem);

    vfs_end_op();
    return ret;
}

/** @} */
