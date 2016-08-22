Virtual Filesystem
==================

Virtual Filesystem (VFS) is a subsystem which unifies all available filesystems
into one big virtual filesystem. All filesystem operations are done through it.
Different mounted filesystems are referenced with names, which are called
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

VFS has two primary data structures: the table of all mounted filesystems and
the table of open files.

The table of all filesystems, ``vfs_table``, is structured as follows:


+-----------------------------------------+-----------------+-------------------------------+
| Type                                    | Name            | Description                   |
+=========================================+=================+===============================+
| ``sempahore_t *``                       | ``sem``         | A binary semaphore used for   |
|                                         |                 | exclusive access to the       |
|                                         |                 | filesystems table.            |
+-----------------------------------------+-----------------+-------------------------------+
| ``vfs_entry_t[CONFIG_MAX_FILESYSTEMS]`` | ``filesystems`` | The filesystems table itself. |
+-----------------------------------------+-----------------+-------------------------------+

A ``vfs_entry_t`` itself has the following fields:

+---------------------------+----------------+--------------------------------+
| Type                      | Name           | Description                    |
+===========================+================+================================+
| ``fs_t *``                | ``filesystem`` | The filesystem driver for this |
|                           |                | mount-point. If ``NULL``, this |
|                           |                | entry is unused.               |
+---------------------------+----------------+--------------------------------+
| ``char[VFS_NAME_LENGTH]`` | ``mountpoint`` | The name of this mount-point.  |
+---------------------------+----------------+--------------------------------+

The table is initialized to contain only ``NULL`` filesystems. All access to
this table must be protected by acquiring the semaphore used to lock the table
(``vfs_table.sem``). New filesystems can be added to this table whenever there
are free rows, but only filesystems with no open files can be removed from the
table.

The table of open files (``openfile_table``) is structured as follows:

+---------------------------------------------+-----------+-----------------------+
| Type                                        | Name      | Description           |
+=============================================+===========+=======================+
| ``semaphore_t *``                           | ``sem``   | A binary semaphore    |
|                                             |           | used for exclusive    |
|                                             |           | access to this        |
|                                             |           | table.                |
+---------------------------------------------+-----------+-----------------------+
| ``openfile_entry_t[CONFIG_MAX_OPEN_FILES]`` | ``files`` | Table of open files.  |
+---------------------------------------------+-----------+-----------------------+

The open files table is also protected by a semaphore (``openfile_table.sem``).
Whenever the table is altered, this semaphore must be held.

An ``openfile_entry_t`` itself has the following fields:

+-------------+-------------------+-------------------------------------+
| Type        | Name              | Description                         |
+=============+===================+=====================================+
| ``fs_t *``  | ``filesystem``    | The filesystem in which this        |
|             |                   | open file is located. If ``NULL``,  |
|             |                   | this is a free entry.               |
+-------------+-------------------+-------------------------------------+
| ``int``     | ``fileid``        | A filesystem defined id for         |
|             |                   | this open file. Every file in a     |
|             |                   | filesystem must have a              |
|             |                   | unique id. Ids do not need          |
|             |                   | to be globally unique.              |
+-------------+-------------------+-------------------------------------+
| ``int``     | ``seek_position`` | The current seek position in        |
|             |                   | the file.                           |
+-------------+-------------------+-------------------------------------+

If access to both tables is needed, the semaphore for ``vfs_table`` must be
held before the ``openfile_table`` semaphore can be lowered. This convention is
used to prevent deadlocks.

In addition to these, VFS uses two semaphores and two integer variables to
track active filesystem operations. The first semaphore is ``vfs_op_sem``,
which is used as a lock to synchronize access to the three other variables. The
second semaphore, ``vfs_unmount_sem``, is used to signal pending unmount
operations when the VFS becomes idle.

The initial value of ``vfs_op_sem`` is one and ``vfs_unmount_sem`` is initially
zero. The integer ``vfs_ops`` is a zero initialized counter which indicates the
number of active filesystem operations on any given moment. Finally, the
boolean ``vfs_usable`` indicates whether VFS subsystem is in use. VFS is out of
use before it has been initialized and it is turned out of use when a forceful
unmount is started by the shutdown process.

.. _vfs_operations:

VFS Operations
--------------

The virtual filesystem is initialized at the system bootup by calling the
following function:

``void vfs_init(void)``
  * Initializes the virtual filesystem. This function is called before virtual
    memory is initialized.
  * Implementation:

    1. Create the semaphore ``vfs_table.sem`` (initial value 1) and the semaphore
       ``openfile_table.sem`` (initial value 1).
    2. Set all entries in both ``vfs_table`` and ``openfile_table`` to free.
    3. Create the semaphore ``vfs_op_sem`` (initial value 1) and the semaphore
       ``vfs_unmount_sem`` (initial value 0).
    4. Set the number of active operations (``vfs_ops``) to zero.
    5. Set the VFS usable flag (``vfs_usable``).

When the system is being shut down, the following function is called to unmount
all filesystems:

``void vfs_deinit(void)``
  * Force unmounts on all filesystems. This function must be used only at
    system shutdown.
  * Sets VFS into unusable state and waits until all active filesystem
    operations have been completed. After that, unmounts all filesystems.
  * Implementation:

    1. Call ``semaphore_P`` on ``vfs_op_sem``.
    2. Set VFS usable flag to false.
    3. If there are active operations (``vfs_ops`` > 0): call ``semaphore_V``
       on ``vfs_op_sem``, wait for operations to complete by calling
       ``semaphore_P`` on ``vfs_unmount_sem``, re-acquire the ``vfs_op_sem``
       with a call to ``semaphore_P``.
    4. Lock both data tables by calling ``semaphore_P`` on both
       ``vfs_table.sem`` and ``openfile_table.sem``.
    5. Loop through all filesystems and unmount them.
    6. Release semaphores by calling ``semaphore_V`` on
       ``openfile_table.sem``, ``vfs_table.sem`` and ``vfs_op_sem``.

To maintain count on active filesystem operations and to wake up pending
forceful unmount, the following two internal functions are used. The first one
is always called before any filesystem operation is started and the latter when
the operation has finished.

``static int vfs_start_op(void)``
  * Start a new VFS operation. A VFS operation is anything that touches a
    filesystem.
  * Returns ``VFS_OK`` if the operation can continue, or error (negative value)
    if the operation cannot be started (VFS is unusable). If the operation
    cannot continue, it should not later call ``vfs_end_op``.
  * Implementation:

    1. Call ``semaphore_P`` on ``vfs_op_sem``.
    2. If VFS is usable, increment ``vfs_ops`` by one.
    3. Call ``semaphore_V`` on ``vfs_op_sem``.
    4. If VFS was usable, return ``VFS_OK``, else return ``VFS_UNUSABLE``.

``static void vfs_end_op(void)``
  * End a started VFS operation.
  * Implementation:

    1. Call ``semaphore_P`` on ``vfs_op_sem``.
    2. Decrement ``vfs_ops`` by one.
    3. If VFS is not usable and the number of active operations is zero, wake
       up pending forceful unmount by calling ``semaphore_V`` on
       ``vfs_unmount sem``.
    4. Call ``semaphore_V`` on ``vfs_op_sem``.

.. _file_operations:

File Operations
---------------

The primary function of the virtual filesystem is to provide unified access to
all mounted filesystems. The filesystems are accessed through file operation
functions.

Before a file can be read or written it must be opened by calling ``vfs_open``:

``openfile_t vfs_open(char *pathname)``
  * Opens the file addressed by ``pathname``. The name must include both the full
    pathname and the filename. (e.g. ``[root]shell.mips32``)
  * Returns an open file identifier. Open file identifiers are non-negative
    integers. On error, negative value is returned.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Parse ``pathname`` into volume name and filename parts.
    3. If filename is not valid (too long, no mount point, etc.), call
       ``vfs_end_op`` and return with error code ``VFS_ERROR``.
    4. Acquire locks to the filesystem table and the open file table.
    5. Find a free entry in the open file table. If no free entry is found (the
       table is full), free the locks, call ``vfs_end_op``, and return with the
       error code ``VFS_LIMIT``.
    6. Find the filesystem specified by the volume name part of the pathname
       in the filesystem table. If the volume is not found, return with the same
       procedure as for a full open file table, except that the error code is
       ``VFS_NO_SUCH_FS``.
    7. Allocate the found free open file entry by setting its filesystem field.
    8. Free the filesystem and the open file table locks.
    9. Call the filesystem's internal open function. If the return value
       indicates an error, lock the open file table, mark the entry free and
       free the lock. Call ``vfs_end_op`` and return the error given by the
       filesystem.
    10. Save the file identifier returned by the filesystem in the
        open file table.
    11. Set file's seek position to zero (beginning of the file).
    12. Call ``vfs_end_op``.
    13. Return the row number in the open file table as the open file
        identifier.

Open files must be properly closed. If a filesystem has open files, the
filesystem cannot be unmounted except on shutdown where unmount is forced. The
closing is done by calling ``vfs_close``:

``int vfs_close(openfile_t file)``
  * Closes an open file ``file``.
  * Returns ``VFS_OK`` (zero) on success, negative on error.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Lock the open file table.
    3. Verify that the given file is really open, otherwise, free the open file
       table lock and return ``VFS_INVALID_PARAMS``.
    4. Call close on the actual filesystem for the file.
    5. Mark the entry in the open file table free.
    6. Free the open file table lock.
    7. Call ``vfs_end_op``.
    8. Return the return value given by the filesystem when close was called.

The seek position within the file can be changed by calling:

``int vfs_seek(openfile_t file, int seek position)``
  * Seek the given open file to the given seek position.
  * The position is not verified to be within the file's size and behavior on
    exceeding the current size of the file is filesystem dependent.
  * Returns ``VFS_OK`` on success, negative on error.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Lock the open file table.
    3. Verify that the given file is really open, otherwise, free the open file
       table lock and return ``VFS_INVALID_PARAMS``.
    4. Set the new seek position in open file table.
    5. Free the open file table.
    6. Call ``vfs_end_op``.
    7. Return ``VFS_OK``.

``int vfs_read(openfile_t file, void *buffer, int bufsize)``
  * Reads at most ``bufsize`` bytes from the given file into the buffer. The read
    is started from the current seek position and the seek position is updated to
    match the new position in the file after the read.
  * Returns the number of bytes actually read. On most filesystems, the
    requested number of bytes is always read when available, but this behaviour
    is not guaranteed. At least one byte is always read, unless the end of file
    or error is encountered. Zero indicates the end of file and negative values
    are errors.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Verify that the given file is really open, otherwise return
       ``VFS_INVALID_PARAMS``.
    3. Call the internal ``read`` function of the filesystem.
    4. Lock the open file table.
    5. Update the seek position in the open file table.
    6. Free the open file table.
    7. Call ``vfs_end_op``.
    8. Return the value returned by the filesystem's ``read``.

``int vfs_write(openfile_t file, void *buffer, int datasize)``
  * Writes at most ``datasize`` bytes from the given ``buffer`` into the open
    ``file``.
  * The write is started from the current seek position and the seek position
    is updated to match the new place in the file.
  * Returns the number of bytes written. All bytes are always written unless
    an unrecoverable error occurs (filesystem full, for example). Negative
    values are error conditions on which nothing was written.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Verify that the given file is really open, otherwise return
       ``VFS_INVALID_PARAMS``.
    3. Call the internal ``write`` function of the filesystem.
    4. Lock the open file table.
    5. Update the seek position in the open file table.
    6. Free the open file table.
    7. Call ``vfs_end_op``.
    8. Return the value returned by the filesystem's ``write``.

Files can be created and removed by the following two functions:

``int vfs_create(char *pathname, int size)``
  * Creates a new file with given ``pathname``. The size of the file will be
    ``size``. The ``pathname`` must include the mount-point (full name would
    be ``[root]shell.mips32``, for example).
  * Returns ``VFS_OK`` on success, negative on error.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Parse the ``pathname`` into volume name and file name parts.
    3. If the ``pathname`` was badly formatted or too long, call ``vfs_end_op``
       and return with the error code ``VFS_ERROR``.
    4. Lock the filesystem table. (This is to prevent unmounting of the
       filesystem during the operation. Unlike read or write, we do not have an
       open file to guarantee that unmount does not happen.)
    5. Find the filesystem from the filesystem table. If it is not found, free
       the table, call ``vfs_end_op`` and return with the error code
       ``VFS_NO_SUCH_FS``.
    6. Call the internal ``create`` function of the filesystem.
    7. Free the filesystem table.
    8. Call ``vfs_end_op``.
    9. Return the value returned by the filesystem's ``create``.

``int vfs_remove(char *pathname)``
  * Removes the file with the given ``pathname``. The pathname must include the
    mount-point (a full name would be ``[root]shell.mips32``, for example).
  * Returns ``VFS_OK`` on success, negative on failure.
  * Implementation:

    1. Call ``vfs_start_op``. If an error is returned by it, return immediately
       with the error code ``VFS_UNUSABLE``.
    2. Parse the pathname into the volume name and file name parts.
    3. If the ``pathname`` was badly formatted or too long, call ``vfs_end_op``
       and return with the error code ``VFS_ERROR``.
    4. Lock the filesystem table. (This is to prevent unmounting of the
       filesystem during the operation. Unlike read or write, we do not have an
       open file to guarantee that unmount does not happen.)
    5. Find the filesystem from the filesystem table. If it is not found, free
       the filesystem table, call vfs end op and return with the error code
       ``VFS_NO_SUCH_FS``.
    6. Call the internal ``remove`` function of the filesystem.
    7. Free the filesystem table by calling semaphore V on vfs table.sem.
    8. Call ``vfs_end_op``.
    9. Return the value returned by the filesystem's ``remove``.

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
