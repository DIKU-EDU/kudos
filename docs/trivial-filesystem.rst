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

