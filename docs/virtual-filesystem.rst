Virtual Filesystem
==================

Virtual Filesystem (VFS) is a subsystem which unifies all available filesystems
into one big virtual filesystem. All filesystem operations are done through it.
Different attached filesystems are referenced with names, which are called
mount-points or volumes.

VFS provides a set of file access functions (see :ref:`file_operations`) and a
set of filesystem access functions (see :ref:`filesystem_operations`). The file
access functions can be used to open files on any filesystem, close open files,
read and write open files, create new files and delete existing files.

The filesystem manipulation functions are used to mount filesystems into VFS,
unmount filesystems, and get information about mounted filesystems (e.g. the
amount of free space on a volume). A mechanism for forceful unmounting of all
filesystems is also provided. This mechanism is needed when the system performs
shutdown, to prevent filesystem corruption.

To be able to provide these services, VFS keeps track of mounted filesystems
and open files. VFS is thread-safe and synchronizes all its own operations and
data structures.  However TFS, which is accessed through VFS, does not provide
proper concurrent access, it simply allows only one operation at a time.

Return Values
-------------

All VFS operations return non-negative values as an indication of successful
operation and negative values as failures. The return value ``VFS_OK`` is
defined to be zero, and indicates success. The rest of the pre-defined return
values are negative. The full list of is as follows:

``VFS_OK``
  The operation succeeded.

``VFS_NOT_SUPPORTED``
  The requested operation is not supported and thus failed.

``VFS_INVALID_PARAMS``
  The parameters given to the called function were invalid and the operation
  failed.

``VFS_NOT_OPEN``
  The operation was attempted on a file which was not open and thus failed.

``VFS_NOT_FOUND``
  The requested file or directory does not exist.

``VFS_NO_SUCH_FS``
  The referenced filesystem or mount-point does not exist.

``VFS_LIMIT``
  The operation failed because some internal limit was hit. Typically this
  limit is the maximum number of open files or the maximum number of mounted
  filesystems.

``VFS_IN_USE``
  The operation couldn't be performed because the resource was busy.
  (Filesystem unmounting was attempted when filesystem has open files, for
  example.)

``VFS_ERROR``
  Generic error, might be hardware related.

``VFS_UNUSABLE``
  The VFS is not in use, probably because a forceful unmount has been requested
  by the system shutdown code.

Limits
------

VFS limits the length of strings in filesystem operations. Filesystem
implementations and VFS file and filesystem access users must make sure to use
these limits when interacting with VFS.

The maximum length of a filename is defined to be 15 characters plus one
character for the end of string marker, i.e. ``VFS_NAME_LENGTH`` is set to 16.

The maximum path length, including the volume name (mount-point), possible
absolute directory path and filename is defined to be 255 plus one character
for the end of string marker, i.e. ``VFS_PATH_LENGTH`` is set to 256.

Internal Data Structures
------------------------

.. _vfs_operations:

VFS Operations
--------------

.. _file_operations:

File Operations
---------------

.. _filesystem_operations:

Filesystem Operations
---------------------

In addition to providing an unified access to all filesystems, VFS also
provides functions to mount and unmount filesystems. Filesystems are
automatically mounted at boot time with the function ``vfs_mount_all``, which
is described below.

The file ``kudos/fs/filesystems.c`` contains a table of all available
filesystem drivers. When an automatic mount is attempted, this table is
traversed by the ``filesystems_try_all`` function to find a driver that matches
the filesystem on the disk, if any.

``void vfs_mount_all(void)``
  * Mounts all filesystems found on all disks attached to the system. Tries all
    known filesystems until a match is found. If no match is found, prints a
    warning and ignores the disk in question.
  * Called in the system boot up sequence.
  * Implementation:

    1. For each disk in the system do the following steps:
      a. Get the device entry for the disk by calling ``device_get``.
      b. Dig the generic block device entry from the device descriptor.
      c. Attempt to mount the filesystem on the disk by calling
         ``vfs_mount_fs`` with ``NULL`` as the volumename (see below).

To attach a filesystem manually either of the following two functions can be
used. The first one probes all available filesystem drivers to initialize one
on the given disk and the latter requires the filesystem driver to be
pre-initialized.

``int vfs_mount_fs(gbd_t *disk, char *volumename)``
  * Mounts the given disk to the given mountpoint (``volumename``).
    ``volumename`` must be non-empty. The mount is performed by trying
    out all available filesystem drivers listed in the ``filesystems`` array
    in ``kudos/fs/fileystems.c``. The first match (if any) is used as the
    filesystem driver for the disk.
  * If ``NULL`` is given as the ``volumename``, the name returned by the
    filesystem driver is used as the mount-point.
  * Returns ``VFS_OK`` (zero) on success, negative on error (no matching
    filesystem driver or too many mounted filesystems).
  * Implementation:

      1. Try the ``init`` functions of all available filesystems in
         ``kudos/fs/filesystems.c`` by calling ``filesystems_try_all``.
      2. If no matching filesystem driver was found, print warning and
         return the error code ``VFS_NO_SUCH_FS``.
      3. If the ``volumename`` is ``NULL``, use the name stored into
         ``fs_t->volume`` name by the filesystem driver.
      4. If the ``volumename`` is an empty string, unmount the filesystem
         driver from the disk and return ``VFS_INVALID_PARAMS``.
      5. Call ``vfs_mount`` (see below) with the filesystem driver instance
         and ``volumename``.
      6. If ``vfs_mount`` returned an error, unmount the filesystem driver
         from the disk and return the error code given by it.
      7. Return with ``VFS_OK``.

``int vfs_mount(fs_t *fs, char *name)``
  * Mounts an initialized filesystem driver ``fs`` into the VFS mount-point
    ``name``.
  * Returns ``VFS_OK`` on success, negative on error. Typical errors are
    ``VFS_LIMIT`` (too many mounted filesystems) and ``VFS-ERROR``
    (mount-point was already in use).
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return
       immediately with the error code ``VFS_UNUSABLE``.
    2. Lock the filesystem table by calling ``semaphore_P`` on
       ``vfs_table.sem``.
    3. Find a free entry on the filesystem table.
    4. If the table was full, free it by calling ``semaphore_V`` on
       ``vfs_table.sem``, call ``vfs_end_op`` and return the error
       code ``VFS_LIMIT``.
    5. Verify that the mount-point name is not in use. If it is, free
       the filesystem table by calling ``semaphore_V`` on ``vfs_table.sem``,
       call ``vfs_end_op`` and return the error code ``VFS_ERROR``.
    6. Set the ``mountpoint`` and ``fs`` fields in the filesystem table to
       match this mount.
    7. Free the filesystem table by calling ``semaphore_V`` on ``vfs_table.sem``.
    8. Call ``vfs_end_op``.
    9. Return ``VFS_OK``.

To find out the amount of free space on given filesystem volume, the following
function can be used:

``int vfs_getfree (char *filesystem)``
  * Finds out the number of free bytes on the given filesystem, identified by
    its mount-point name.
  * Returns the number of free bytes, negative values are errors.
  * Implementation:

      1. Call ``vfs_start_op``. If an error is returned by it, return
         immediately with the error code ``VFS_UNUSABLE``.
      2. Lock the filesystem table by calling ``semaphore_P`` on
         ``vfs_table.sem``. (This is to prevent unmounting of the filesystem
         during the operation. Unlike read or write, we do not have an open
         file to guarantee that unmount does not happen.)
      3. Find the filesystem by its mount-point name ``filesystem``.
      4. If the filesystem is not found, free the filesystem table by calling
         ``semaphore_V`` on ``vfs_table.sem``, call ``vfs_end_op`` and return
         the error code ``VFS_NO_SUCH_FS``.
     5. Call filesystem's ``getfree`` function.
     6. Free the filesystem table by calling ``semaphore_V`` on
        ``vfs_table.sem``
     7. Call ``vfs_end_op``.
     8. Return the value returned by filesystem's ``getfree`` function.
