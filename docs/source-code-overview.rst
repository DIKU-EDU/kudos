Source Code Overview
====================

..
   An operating system kernel is the core of any OS.  The kernel acts as a glue
   between userland processes and system hardware, providing an illusion of
   exclusive access to system resources.  Each userland program is run in a
   private sandbox, and processes should be able to interact only through well
   defined means, i.e. system calls.

   The KUDOS kernel is multithreaded and can use multiple CPUs.  The kernel
   provides the threading and synchronization primitives.  Several device
   drivers for the simulated devices of Yams are also provided.  Memory
   handling in the kernel is quite primitive, as most virtual memory features
   are left as exercises.  The system has a simple filesystem and support for
   multiple filesystems.

   Userland programs are somewhat supported, but proper system call handling
   and proper user processes (and not just kernel threads) are left as
   exercises -- more about that later.

   Directory structure
   -------------------

The KUDOS source code is split into two main subdirectories:

1. :ref:`kudos` -- operating system proper, and
2. :ref:`userland` -- containing userland programs.

The KUDOS source code also contains a subdirectory called ``tools``, containing
useful tools for running KUDOS (see :doc:`using-kudos` for an overview), and
``docs``, containing the source code for this documentation. You should not
need to touch either of these subdirectories.

.. _kudos:

``kudos``
---------

The kernel source code is split into modules stored in subdirectories. A module
typically consists of some C-files and a ``module.mk``. To add a new module,
list it in the ``MODULES`` variables in ``kudos/Makefile``.  To add new C-files
to a module, list them in the ``FILES`` variable of the ``module.mk``. A module
may also contain architecture-specific implementations in designated
sub-subdirectories, such as ``mips32`` and ``x86-64``.

Currently, the kernel contains the following modules:

``kudos/init/``
~~~~~~~~~~~~~~~

Kernel initialization and entry point.  This directory contains the functions
that KUDOS will execute to bootstrap itself. See :doc:`bootstrapping-kudos` for
documentation of this module.

``kudos/kernel/``
~~~~~~~~~~~~~~~~~

Thread handling, context switching, scheduling and synchronization.  Also
various core functions used in the KUDOS kernel reside here (e.g. ``panic`` and
``kmalloc``). Documentation coming soon.

``kudos/proc``
~~~~~~~~~~~~~~

Userland processes.  Starting of new userland processes, loading userland
binaries and handling exceptions and system calls from userland. See
:doc:`system-calls` for documentation about the system call interface.
Documentation about the rest of this module is coming soon.

``kudos/vm``
~~~~~~~~~~~~

Virtual memory subsystem.  Managing the available physical memory and page
tables. Documentation coming soon.

``kudos/fs``
~~~~~~~~~~~~

Filesystem abstractions and the Trivial Filesystem (TFS). Documentation coming
soon.

``kudos/drivers``
~~~~~~~~~~~~~~~~~

Low level device drivers and their interfaces. See :doc:`device-drivers` and
:doc:`builtin-drivers` for documentation of this module.

``kudos/lib``
~~~~~~~~~~~~~

Miscellaneous library code (``kwrite``, ``kprintf``, various string-handling
functions, a random number generator, etc.).  Documentation coming soon.

``kudos/util``
~~~~~~~~~~~~~~

Utilities for using KUDOS (e.g. ``tfstool`` used for writing the Trivial
Filesystem disk files). See :doc:`appendix` for more information about
``tfstool``.

.. _userland:

``userland``
------------

Userland programs.  These are not part of the kernel.  They can be used to test
the userland implementation of KUDOS by saving them to a Trivial Filesystem
disk file and booting KUDOS with that. See :doc:`using-kudos` for information
on how to do that.
