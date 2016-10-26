Using KUDOS
===========

The KUDOS system requires the following software to run:

* Qemu
* GNU Binutils
* GNU GCC 
* GNU Make

Note that KUDOS also has an ``x86-64`` target.

Compiling the kernel
--------------------

You can compile the operating system by running ``make`` in the ``kudos/``
subdirectory of KUDOS.

After compiling the system, you should have a binary named ``kudos-x86_64`` in
that directory.  This is your entire operating system, in one file!


Compiling the userland programs
-------------------------------

Userland programs are compiled using the same compiler used for compiling
KUDOS.

To compile userland binaries, go to the ``userland/`` subdirectory of KUDOS and
run ``make``.

To run these programs in KUDOS, they need to be copied to a virtaul
disk, where KUDOS can find them.

If you wish to add your own userland binary, list the source files in the
``SOURCES`` variable at the beginning of your ``userland/Makefile``.


Writing to the virtual disk
---------------------------

KUDOS has a The Trivial Filesystem (TFS) implementation and a tool ``tfstool``
for managing TFS volumes.  To get a summary of the arguments that ``tfstool``
accepts, you can run it without any arguments.

``tfstool`` itself is documented in the :doc:`appendix`.

By default, the file containing the TFS volume is named ``store.file``.


Booting the system
------------------
Before KUDOS can be booted, you have to compile both KUDOS and the userland
programs. When that is done, you have to create a virtual disk and
copy the userland program to the virtual disk.

To boot KUDOS we need to open a terminal window, and change the directory
to the KUDOS directory. When the directory is change, run
``./run_qemu.sh``.
A window should open and show you a boot menu. By pressing ``e``
you can change the kernel parameters that KUDOS uses. Currently
KUDOS supports the following kernel parameters:

* ``initprog``: the name of the file in the YAMS disk that the kernel starts at
  the first thread.
* ``randomseed``: the initial seed for KUDOS' random number generator.

Example: Compile and run ``halt``
---------------------------------

In this subsection, we will go through the compilation and running of the
``halt`` userland program handed out together with KUDOS.

Once you have a version of KUDOS extracted on your system, build the kernel and
the userland programs::

    ~/kudos$ make -C kudos
    ~/kudos$ make -C userland

Then transfer the userland program ``halt`` onto the ``store.file`` virtual disk::

    ~/kudos$ ./kudos/util/tfstool write store.file userland/halt.x86_64 halt

To start qemu and boot qemu::

    ~/kudos$ ./run_qemu.sh

This should open a new qemu window and boot KUDOS.

Run ``./kudos/util/tfstool list store.file`` to list the files currently stored in the KUDOS TFS
disk.
