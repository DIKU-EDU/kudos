Bootstrapping KUDOS
===================

This section explains the bootup process of KUDOS from the first instruction
ever executed by the CPU to the point when userland processes can be started.
This is **not** an introduction to *running* KUDOS in YAMS, for that, we refer
you to :doc:`using-kudos`.

In the beginning there was ``_boot.S``
--------------------------------------

When YAMS is powered up, its program counter is set to value ``0x80010000`` for
all processors. This is the address where the KUDOS binary image is also
loaded. Code in ``kudos/init/_boot.S`` is the very first code that YAMS will
execute.  Because no initializations are done (ie. there is no stack),
``_boot.S`` is written in assembly.

The first thing that the ``boot.S`` code will do is processor separation. The
processor number is detected and all processors except number 0 will enter a
wait loop waiting for the kernel initialization to be finished. Later, when the
kernel initialization (in ``main.c``) is finished, processor 0 will signal the
other processors to continue.

So that further initialization code could be written in a high-level language,
we need a stack. A temporary stack will be located at address ``0x8000fffc``,
just below the starting point of the KUDOS binary image. The stack will grow
downward. Setting up the base address of the stack is done after processor
separation in ``_boot.S``. Later, after initialization code, every kernel
thread will have own stack areas.

After this we have a stack and high-level language routines may be used. On the
next line of ``_boot.S``, we’ll jump to the high-level initialization code
``init()`` located in ``kudos/init/main.c``.

Starting Subsystems
-------------------

To provide operating system service, various subsystems are required, in order
to coordinate resource usage. These are started in the respective,
architecture-specific ``init`` functions in ``init/$ARCH/main.c``.

``stalloc``
  Provides permanent, static kernel memory allocation.

``polltty``
  Allows input from the keyboard, and text output to the display. This is a
  *polling* device, so getting input/putting output requires repeatedly checking
  the status of the keyboard/display in a busy-wait loop.

``interrupt``
  Allows devices and user processes to *interrupt* the CPU when they have an
  event that must be handled. This can be used to prevent the CPU from having to
  poll. The same subsystem handles *exceptions*.

``thread_table``
  Allows the operating system to have mutiple threads of execution. It is
  initialised, by creating a *thread table* to keep track of the threads. A
  *thread* of execution is (the state of) some running code, e.g. registers such
  as the instruction pointer.

``sleep_queue``
  A syncronization mechanism, allowing threads to wait on resources currently
  in use by other threads.

``semaphore``
  Another, high-level, inter-thread syncronization mechanism.

``device``
  Stores information about I/O devices, such as the TTY device, in a *device
  table*. In KUDOS, devices implement generic interfaces, minimizing code
  duplication.

``vfs``
  The Virtual File-System (VFS) is a generic file-system abstraction over more
  specific file-systems, such as the KUDOS Trivial File System (TFS).

``sheduler``
  Performs the actual switching of threads on and off CPU cores, saving the
  context of the thread, such as its registers.

``vm``
  Allows the operating system to place pages non-contiguously in memory, by
  mapping addresses from *physical addresses* to *virtual addresses*. Once this
  subsystem has been initialised, the ``stalloc`` subsystem is disabled.

Finally, a new thread is created and run on the CPU instead of the current
thread. On ``kudos-mips32`` the other CPUs are now released from the wait-loop.
The new thread executes the architecture-independent function
``init_startup_thread`` (defined in ``kudos/init/common.c``) which sets up the
last two things:

1. All filesystems implementing the VFS interface are made available, or
   mounted, by ``vfs_mount_all``.

2. The program corresponding to the ``initprog`` argument given at boot is
   loaded into memory, and execution continues at the address of the first
   instruction in this program.  If no ``initprog`` argument was given, the
   function ``init_startup_fallback`` is called instead.
