.. role:: c(code)
   :language: c

Filesystems
===========

A "filesystem" is an organization of "files" into a system, often backed by
some sort of long-term storage device. Before the kernel or userland can
perform file operations, the filesystem has to be properly "mounted". Modern
operating systems support the mounting of multiple filesystems, and provide a
virtual filesystem layer.

KUDOS is no exception.

KUDOS supports one filesystem, called the :doc:`trivial-filesystem`.
Filesystems are managed and accessed through a layer called the
:doc:`virtual-filesystem` layer which represents a union of all mounted
filesystems.

The Trivial Filesystem supports only the most primitive filesystem operations
and does not enable concurrent access to the filesystem. Only one request
(read, write, create, open, close, etc.) is allowed to be in action at any
given time. TFS enforces this restriction internally.

Filesystem Conventions
----------------------

Files on filesystems are addressed by filenames. In KUDOS, filenames can have
at most 15 alphanumeric characters. The full path to a file is called an
absolute pathname and it must contain the volume (mount-point or filesystem) on
which the file is, possibly a directory path, and finally, the name of the
file within that directory.

An example of a valid filename is shell. A full absolute path to a shell might
be ``[disk]shell`` or ``[disk]bin/shell``. Here ``shell`` is the name of a
file, ``disk`` is a volume name (you could also call it a disk, filesystem or
mount-point). If directories are used, ``bin`` is a name of a directory.
Directories have the same restrictions on filenames as files do (directory are
really just files). Directory names in a path are separated by ``/``.

Filesystem Layers
-----------------

Typically a filesystem is located on a disk (but it can also be a network
filesystem or even totally virtual). Disks are accessed through Generic lock
Devices (see :doc:`device-drivers`). At boot time, the system will try to mount
all available filesystem drivers on all available disks through their GBDs. The
mounting is done into a virtual filesystem.

Virtual Filesystem is a super-filesystem which contains all attached (mounted)
filesystems. The same access functions are used to access disk, networked and
fully virtual filesystems. An example of a "fully virtual filesystem" is the
``proc`` filesystem on Linux, which makes a range of process-related
information available from under the ``/proc`` directory. The actual filesystem
driver is recognized from the volume name part of a full absolute pathname
provided to the access functions.

+-----------------------+------------------------------------------+
| Files                 | Purpose                                  |
+=======================+==========================================+
| ``vfs.[hc]``          | :doc:`virtual-filesystem` implementation |
+-----------------------+------------------------------------------+
| ``filesystems.[hc]``  | Available filesystems                    |
+-----------------------+------------------------------------------+
| ``tfs.[hc]``          | :doc:`trivial-filesystem` implementation |
+-----------------------+------------------------------------------+
