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


