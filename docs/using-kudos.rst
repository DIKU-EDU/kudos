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

``tfstool`` itself is documented in the :doc:`appendix`, but we recommend to
first try our ``yams-tfs`` tool which automatically builds all userland
programs, creates a file containing a TFS volume, *and* writes all userland
programs to the new volume.  You can use ``yams-files`` to list the currently
stored files.

By default, the file containing the TFS volume is named ``store.file``.


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

    ~/kudos$ tools/yams-init halt

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
