Code Overview
=============
.. _code-overview:

An operating system kernel is the core of any OS.  The kernel acts as a glue
between userland processes and system hardware, providing an illusion of
exclusive access to system resources.  Each userland program is run in a private
sandbox, and processes should be able to interact only through well defined
means, i.e. system calls.

The KUDOS kernel is multithreaded and can use multiple CPUs.  The kernel
provides the threading and synchronization primitives.  Several device drivers
for the simulated devices of Yams are also provided.  Memory handling in the
kernel is quite primitive, as most virtual memory features are left as
exercises.  The system has a simple filesystem and support for multiple
filesystems.

Userland programs are somewhat supported, but proper system call handling and
proper user processes (and not just kernel threads) are left as exercises --
more about that later.


Directory structure
-------------------

The KUDOS source code consists of modules stored in directories.  A directory
may contain subdirectories for architecture-specific implementations for MIPS
and x86-64.  The directories and their contents are as follows:

``kudos/init/``
~~~~~~~~~~~~~~~

Kernel initialization and entry point.  This directory contains the functions
that KUDOS will execute first when it is booted.

``kudos/kernel/``
~~~~~~~~~~~~~~~~~

Thread handling, context switching, scheduling and synchronization.  Also
various core functions used in the KUDOS kernel reside here (e.g. ``panic`` and
``kmalloc``).

``kudos/proc``
~~~~~~~~~~~~~~

Userland processes.  Starting of new userland processes, loading userland
binaries and handling exceptions and system calls from userland.

``kudos/vm``
~~~~~~~~~~~~

Virtual memory subsystem.  Managing the available physical memory and page
tables.

``kudos/fs``
~~~~~~~~~~~~

Filesystem abstractions and the Trivial Filesystem (TFS).

``kudos/drivers``
~~~~~~~~~~~~~~~~~

Low level device drivers and their interfaces.

``kudos/lib``
~~~~~~~~~~~~~

Miscellaneous library code (e.g. string handling and random number generation).

``kudos/util``
~~~~~~~~~~~~~~

UNIX utilities for KUDOS usage (e.g. ``tfstool`` used for writing the Trivial
Filesystem disk files).

``userland``
~~~~~~~~~~~~

Userland programs.  These are not part of the kernel.  They can be used to test
the userland implementation of KUDOS by saving them to a Trivial Filesystem disk
file and booting KUDOS with that.
