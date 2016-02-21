Appendix
========

More about the ``tfstool``
--------------------------

The utility program, ``tfstool``, which is shipped with KUDOS, provides a way
to transfer files to a filesystem KUDOS understands.  ``tfstool`` can be used
to create a Trivial Filesystem (TFS, documentation about TFS coming soon) to a
given file, to examine the contents of a file containing a TFS, and to transfer
files to the TFS.  KUDOS implementation of TFS does not include a way to
initialize the filesystem, so using ``tfstool`` is the only way to create a new
TFS.  ``tfstool`` is also used to move userland binaries to TFS. When you write
your own filesystem to KUDOS, you might find it helpful to leave TFS intact.
This way you can still use ``tfstool`` to transfer files to the KUDOS system
without writing another utility program for your own filesystem.

The implementation of the ``tfstool`` is provided in the ``kudos/util/``
directory. The KUDOS ``Makefile`` can be used to compile it to the executable
``kudos/util/tfstool``. Note that ``tfstool`` is compiled with the native
compiler, not the cross-compiler used to compile KUDOS. The implementation
takes care of byte-order conversion (big-endian, little-endian) if needed.

To get a summary of the arguments that ``tfstool`` accepts you may run it
without arguments::

  $ kudos/util/tfstool 
  KUDOS Trivial Filesystem (TFS) Tool
  ...

The accepted commands are also explained below:

``create <filename> <size> <volume-name>``

  Create a new TFS volume and write it to file ``<filename>``. The total size of
  the file system will be ``<size>`` 512-byte blocks. Note that the three first
  blocks are needed for the TFS header, the TFS master directory, and the TFS
  block allocation table.  ``<size>`` must therefore be at least 3. The created
  volume will have the name ``<volume-name>``.


  Note that the number of blocks must be the same as the setting in
  ``yams.conf``.

``list <filename>``

  List the files found in the TFS volume residing in ``<filename>``.

``write <filename> <local-filename> [<TFS-filename>]``

  Write a file from the local system (``<local-filename>``) to the TFS volume
  residing in the file ``<filename>``. The optional fourth argument specifies the
  filename to use for the file inside the TFS volume. If not given,
  ``<local-filename>`` will be used.

  Note that you probably want to give a ``<TFS-filename>``, since otherwise you
  end up with a TFS volume with files named like ``userland/halt.mips32``, which
  can cause confusion since **TFS does not support directories**.

``read <filename> <TFS-filename> [<local-filename>]``

  Read a file (``<TFS-filename>``) from TFS volume residing in the file filename
  to the lo- cal system. The optional fourth argument specifies the filename in
  the local system. If not given, the ``<TFS-filename>>`` will be used.

``delete <filename> <TFS-filename>``

  Delete the file with name ``<TFS-filename>`` from the TFS volume residing in
  the file ``<filename>``.

YAMS Configuration
------------------

A sample YAMS configuration file, called ``yams.conf``, is handed out together
with KUDOS. The file is structured into sections as follows:

+---------------+-------------------------------------------------+
| Name          | Purpose                                         |
+===============+=================================================+
| ``simulator`` | Configuration of the simulator itself.          |
+---------------+-------------------------------------------------+
| ``disk``      | Configuration of the attached block devices.    |
+---------------+-------------------------------------------------+
| ``tty``       | Configuration of the attached terminal devices. |
+---------------+-------------------------------------------------+

The following subsections document some of the relevant parts of this
configuration. For a more in-depth documentation of YAMS, we refer you to the
`YAMS 1.4.1 Reference Manual
<https://www.niksula.hut.fi/~buenos/dist/yams-1.4.1.pdf>`_.

Disk filename
^^^^^^^^^^^^^

KUDOS implements a trivial file system (TFS), which can be used to read, write,
and execute files in KUDOS. The ``tfstool`` (documented above) can be used to
create TFS volumes, read from, write to such volumes. A TFS volume is stored in
a file.

YAMS can then attach this file as a block device. This is done by adding a
``disk`` section to the YAMS configuration file. Handout KUDOS will attach a
file named ``store.file``. Change the ``filename`` string if you prefer a
different name, or add another ``disk`` section if you want multiple disks.

Note: the disks should have different volume names.

Number of processors
^^^^^^^^^^^^^^^^^^^^

KUDOS is designed to be a *symmetric multiprocessing (SMP)* system, which means
that it supports multiple CPU cores, having each their own :doc:`thread of
execution <threads>`, while sharing the same physical memory. This can lead to
a whole class of race conditions, where multiple concurrent threads access the
same memory location at (what appears to be) exactly the same time.

To avoid problems with simultaneity you can set the ``cpus`` number to 1 in the
``simulator`` section. Handout KUDOS typically sets this number to 4.
