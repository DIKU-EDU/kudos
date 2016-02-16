Using KUDOS
===========

The KUDOS system requires the following software to run:

* The YAMS machine simulator
* GNU Binutils cross-compiled for the ``mips-elf`` target
* GNU GCC cross-compiled for the ``mips-elf`` target
* GNU Make

Note that KUDOS also has an ``x86-64`` target.  However, this document will not
focus on that, as only the ``mips32`` target works with YAMS.

For the `OSM <http://www.webcitation.org/6eoBjRWvD>`_ course, **please use the
handed out KUDOS and YAMS versions**.

Compiling the kernel
--------------------

You can compile the operating system by running ``make`` in the ``kudos/``
subdirectory of KUDOS.

After compiling the system, you should have a binary named ``kudos-mips32`` in
that directory.  This is your entire operating system, in one file!


Compiling the userland programs
-------------------------------

Userland programs are compiled using the same cross-compiler used for compiling
KUDOS.  To run these cross-compiled programs in KUDOS, they need to be copied
to a virtual disk, where KUDOS can then find them.

To compile userland binaries, go to the ``userland/`` subdirectory of KUDOS and
run ``make``.

If you wish to add your own userland binary, list the source files in the
``SOURCES`` variable at the beginning of your ``userland/Makefile``.


Writing to the virtual disk
---------------------------

KUDOS has a The Trivial Filesystem (TFS) implementation and a tool ``tfstool``
for managing TFS volumes.  To get a summary of the arguments that ``tfstool``
accepts, you can run it without any arguments.

KUDOS also ships with a tool ``yams-tfs``, which automatically builds all
userland programs *and* writes them all to a YAMS disk.  You can use
``yams-files`` to list the currently stored files.

By default, the disk is named ``store.file``.


Booting the system
------------------

To boot in our ``mips32`` system with YAMS, we need two terminal windows: one
for diagnostics, and one for operating system input/output.

YAMS' base tools are named ``yamst`` (for the input/output) and ``yams`` (for
the diagnostics), but usually it's easier to use KUDOS' YAMS tools:

* ``yams-term``: Setup an OS input/output window, and wait for directions from
  the other window (communication goes through a POSIX socket).
* ``yams-sim``: Simulate a kernel in the ``yams-term`` window.
* ``yams-init``: Do the same as ``yams-sim``, but type less and assume more.

In more detail:

``yams-term`` takes no arguments.

``yams-sim`` takes one or more arguments; typically two.  The first argument is
the path to the MIPS kernel.  The remaining arguments are so-called ``bootargs``
of the form ``key=value``, which can be read in the kernel by calling
``bootargs_get(key)``.  A selection of bootargs:

* ``initprog``: the name of the file in the YAMS disk that the kernel starts at
  the first thread.
* ``randomseed``: the initial seed for KUDOS' random number generator.

You implicitly define your own bootargs by using ``bootargs_get`` somewhere in
kernel code.

``yams-init`` takes one argument: the name of your init program.  Meaning,
``yams-init program`` calls ``yams-sim kudos/kudos-mips32
initprog=[disk]program.mips32``.


Example: Compile and run ``halt``
---------------------------------

In this subsection, we will go through the compilation and running of the
``halt`` userland program handed out together with KUDOS.

Once you have a version of KUDOS extracted on your system, build the kernel and
the userland programs::

    ~/kudos$ make -C kudos
    ~/kudos$ make -C userland

Then transfer all userland programs onto the ``store.file`` virtual disk::

    ~/kudos$ tools/yams-tfs

Then prepare YAMS for listening on boot requests::
  
    ~/kudos$ tools/yams-term

Let that wait, and open a new terminal window.  In that, run::

    ~/kudos$ tools/yams-sim kudos/kudos-mips32 initprog=[disk]halt.mips32

This should finish without a hitch and print a lot of diagnostics, while the
``yams-term`` terminal window should print both KUDOS boot messages and KUDOS
halt messages (since it's running a program compiled from ``userland/halt.c``,
which calls ``syscall_halt``).  Instead of writing that long ``yams-sim`` line,
you can just write::

    ~/kudos$ tools/yams-init halt.mips32

which should do the same, except not print as much filler text.

Run ``tools/yams-files`` to list the files currently stored in the KUDOS TFS
disk.  It should at least give you this::

    [disk]halt.mips32
    [disk]hw.mips32
    [disk]shell.mips32

where ``[disk]`` is the volume name.

.. tip:: You can add ``~/kudos/tools/`` to your ``PATH``, and avoid having to
         type the ``tools/`` prefix every time. This is already done in the
         handed out VirtualBox image. On your own machine you can do this by
         adding the following line to your ``~/.bashrc``::

              PATH=$HOME/kudos/tools/:$PATH

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
  KUDOS Trivial Filesystem (TFS) Tool -- Version 1.01
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
  end up with a TFS volume with files named like ``userland/foobar.mips32``,
  which can cause confusion since **TFS does not support directories**.

``read <filename> <TFS-filename> [<local-filename>]``

  Read a file (``<TFS-filename>``) from TFS volume residing in the file filename
  to the lo- cal system. The optional fourth argument specifies the filename in
  the local system. If not given, the ``<TFS-filename>>`` will be used.

``delete <filename> <TFS-filename>``

  Delete the file with name ``<TFS-filename>`` from the TFS volume residing in
  the file ``<filename>``.
