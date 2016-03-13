Trivial Filesystem
==================

Trivial File System (TFS) is, as its name implies, a very simple file system.
All operations are implemented in a straightforward manner without much
consideration for efficiency, there is only simple synchronization and no
bookkeeping for open files, etc. The purpose of the TFS is to give students a
working (although not thread-safe) filesystem and a tool (see
:doc:`using-kudos`) for moving data between TFS and the native filesystem of
the platform on which KUDOS is being developed.

When students implement their own filesystem, the idea is that files can be
moved from the native filesystem to the TFS using the TFS tool, and then they
can be moved to the student filesystem using KUDOS itself. This way students
don't necessarily need to write their own tool(s) for the simulator platform.
It is, of course, perfectly acceptable to write your own tool(s).

Trivial filesystem uses the native block size of a drive (must be predefined).
Each filesystem contains a volume header block (block number 0 on disk). After
header block comes block allocation table (BAT, block number 1), which uses one
block. After that comes the master directory block (MD, block number 2), also
using one block. The rest of the disk is reserved for file header (inode) and
data blocks. The following figure illustrates the structure of a TFS volume:

.. _tfs_figure:

.. figure:: tfs.svg

     An illustration of the disk blocks on a TFS volume.

Note that all multibyte data in TFS is *big-endian*. This is not a problem in
the MIPS32 version of KUDOS, since YAMS is big-endian also, but in the x86-64
version of KUDOS this is a problem, since x86-64 is little-endian. This means
that we need to go through the function ``from_big_endian32`` (defined in
``kudos/lib/libc.h``) when dealing with TFS.  For x86-64 this function
translates the value into little-endian, for MIPS32 this function does nothing.

The volume header block has the following structure. Other data may be present
after these fields, but it is ignored by TFS.

+----------+----------------------------+-------------+----------------------+
| Offset   | Type                       | Name        | Description          |
+==========+============================+=============+======================+
| ``0x00`` | ``uint32_t``               | ``magic``   | Magic number, must   |
|          |                            |             | be 3745 (``0x0EA1``) |
|          |                            |             | TFS volumes.         |
+----------+----------------------------+-------------+----------------------+
| ``0x04`` | ``char[TFS_VOLNAME_MAX]``  | ``volname`` | Name of the volume,  |
|          |                            |             | including the        |
|          |                            |             | terminating zero     |
+----------+----------------------------+-------------+----------------------+

The block allocation table is a bitmap which records the free and reserved
blocks on the disk, one bit per block, 0 meaning free and 1 reserved. For a
512-byte block size, the allocation table can hold 4096 bits, resulting in a
2MB disk. Note that the allocation table includes also the three first blocks,
which are always reserved.

The mater directory consists of a single disk block, containing a table of the
following 20-byte entries. This means that a disk with a 512-byte block size
can have at most 25 files (512/20 = 25.6).

+-----------+----------------------------+------------+-------------------------+
| Offset    | Type                       | Name       | Description             |
+===========+============================+============+=========================+
| ``0x00``  | ``uint32_t``               | ``inode``  | Number of the disk      |
|           |                            |            | block containing the    |
|           |                            |            | file header (inode) of  |
|           |                            |            | this file.              |
+-----------+----------------------------+------------+-------------------------+
| ``0x04``  | ``char[TFS_FILENAME_MAX]`` | ``name``   | Name of the file,       |
|           |                            |            | including the           |
|           |                            |            | terminating zero.       |
+-----------+----------------------------+------------+-------------------------+

This means that the maximum file name length is actually
``TFS_FILENAME_MAX-1``.

A file header block ("inode") describes the location of the file on the disk and its actual size.
The contents of the file is stored to the allocated blocks in the order they appear in the block list
(the first BLOCKSIZE bytes are stored to the first block in the list etc.). A file header block has
the following structure:

With a 512-byte block size, the maximum size of a file is limited to 127 blocks (512/4 − 1) or
65024 bytes.

