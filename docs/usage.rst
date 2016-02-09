Using KUDOS
===========
.. _usage:

The Kudos system requires the following software to run:

* The Yams machine simulator
* GNU Binutils for the ``mips-elf`` target
* GNU GCC cross-compiler for the ``mips-elf`` target
* GNU Make

Note that Kudos also has an ``x86-64`` target.  However, this document will not
focus on that, as only the MIPS target works with Yams.

For the DIKU Operating Systems course, please use the handed out Kudos and
Yams versions.


Compiling the kernel
--------------------

You can compile the skeleton system by running ``make`` in the ``kudos/``
subdirectory of Kudos.

After compiling the system, you should have a binary named ``kudos-mips`` in
that directory.  This is your entire operating system in one file!


Compiling the userland programs
-------------------------------

Userland programs are compiled using the same cross-compiler used for compiling
Kudos.  To run compiled programs, they need to be copied to a virtual disk,
where Kudos can then find them.

To compile userland binaries go to the ``userland/`` subdirectory of Kudos and
run ``make``.

If you wish to add your own userland binaries to the Makefile, add your source
files to the ``SOURCES`` variable at the beginning of the Makefile.


Writing to the virtual disk
---------------------------

Kudos has a The Trivial Filesystem (TFS) implementation and a tool ``tfstool``
to copy binaries from host filesystem to Kudos filesystem.  To get a summary of
the arguments that ``tfstool`` accepts, you can run it without any arguments.

Kudos also ships with a tool ``yams-tfs``, which automatically builds all
userland programs *and* writes them all to a Yams disk.  You can use
``yams-files`` to list the currently stored files.

By default, the disk is named ``store.file``.


Booting the system
------------------

To boot our MIPS system with Yams, we need two terminal windows: one for
diagnostics, and one for operating system input/output.

Yams' base tools are named ``yamst`` (for the input/output) and ``yams`` (for
the diagnostics), but usually it's easier to use Kudos' Yams tools:

* ``yams-term``: Setup a OS input/output window, and wait for directions from
  the other window (communication goes through a POSIX socket).
* ``yams-sim``: Simulate a kernel in the ``yams-term`` window.
* ``yams-init``: Do the same as ``yams-sim``, but type less and assume more.

In more detail:

``yams-term`` takes no arguments.

``yams-sim`` takes one or more arguments; typically two.  The first argument is
the path to the MIPS kernel.  The remaining arguments are so-called ``bootargs``
of the form ``key=value``, which can be read in the kernel by calling
``bootargs_get(key)``.  A selection of bootargs:

* ``initprog``: the name of the file in the Yams disk that the kernel starts at
  the first thread.
* ``randomseed``: the initial seed for Kudos' random number generator.

You implicitly define your own bootargs by using ``bootargs_get`` somewhere in
kernel code.

``yams-init`` takes one argument: the name of your init program.  Meaning,
``yams-init program.mips`` calls ``yams-sim kudos/kudos-mips
initprog=[disk]program.mips``.


Example: Compile and run ``halt``
---------------------------------

In this subsection, we will go through the compilation and running of the
``halt`` userland program handed out together with Kudos.

Once you have a version of Kudos extracted on your system, build the kernel and
the userland programs::

    make -C kudos
    make -C userland

Then transfer all userland programs onto the ``store.file`` virtual disk::

    yams-tfs

Then prepare Yams for listening on boot requests::
  
    yams-term

Let that wait, and open a new terminal window.  In that, run::

    yams-sim kudos/kudos-mips initprog=[disk]halt.mips
  
This should finish without a hitch and print a lot of diagnostics, while the
``yams-term`` terminal window should print both Kudos boot messages and
Kudos halt messages (since it's running a program compiled from
`userland/halt.c`, which calls the HALT syscall).  Instead of writing that long
`yams-sim` line, you can just write::

    yams-init halt.mips

which should do the same, except not print as much filler text.

Run ``yams-files`` to list the files currently stored in the Kudos TFS disk.  It
should at least give you this::

    [disk]halt.mips
    [disk]halt.x86_64

where ``[disk]`` is the volume name.
