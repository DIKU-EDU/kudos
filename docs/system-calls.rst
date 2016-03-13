System Calls
============

System calls are an interface through which userland programs can call kernel
functions, mainly those that are I/O-related, and thus require kernel mode
privileges. Userland code cannot of course call kernel functions directly,
since this would imply access to kernel memory, which would break the userland
sandbox and userland programs could corrupt the kernel at their whim. This
means that the system call handlers in the kernel should be written very
carefully. A userland program should not be able to affect normal kernel
functionality *no matter what arguments it passes to the system call* (this is
called *bullet proofing* the system calls).

How System Calls Work
---------------------

A system call is made by first placing the arguments for the system call and
the *system call function number* in predefined registers. In KUDOS, the
standard MIPS32 argument registers ``a0``, ``a1``, ``a3``, and ``a3`` are used
for this purpose. The system call number is placed in ``a0``, and its three
arguments in ``a1``, ``a2`` and ``a3``. If there is a need to pass more
arguments for a system call, this can be easily achieved by making one of the
arguments a memory pointer which points to a structure containing rest of the
arguments. The return value of the system call is placed in a predefined
register by the system call handler. In KUDOS the standard return value
register ``v0`` is used.

After the arguments are in place, the special machine instruction ``syscall``
is executed. The ``syscall`` instruction, in one, atomic instruction, does the
following:

1. sets the CPU core into **kernel mode**,
2. **disables interrupts**, and
3. causes a system call exception.

When an instruction causes an exception, while the CPU core is in user mode,
control is transfered to the user exception handler (defined in
``kudos/proc/$ARCH/exception.c``).

The system call exception is then handled as follows (note that not all details
are mentioned here):

1. The context is saved as with any exception or interrupt.

2. As we notice that the cause of the exception was a system call, interrupts
   are enabled and the system call handler is called. Enabling interrupts (and
   also clearing the EXL bit) results in the thread running as a normal thread
   rather than an exception handler.

3. The system call handler gets a pointer to the user context as its argument.
   The system call number and arguments are read from the registers saved in
   the user context, and an appropriate handler function is called for each
   system call number. The return value is then written to the ``v0`` register
   saved in the user context.

4. The program counter in the saved user context is incremented by one
   instruction, since it points to the ``syscall`` instruction which generated
   this exception.

5. Interrupts are disabled (and EXL bit set), and the thread is again running
   as an exception handler.

6. The context is restored, which also restores the thread to user mode.

The last step above uses a "return from exception" instruction, ``eret``, which
in one, atomic instruction, does the following:

1. clears the exception flag,
2. **enables interrupts**,
3. sets the CPU core into **user mode**, and finally,
4. jumps to the address in the ``EPC`` register on MIPS co-processor 0.

**Note:** You cannot directly change thread/process (i.e. call scheduler) when
in syscall or other exception handlers, since it will mess up the stack. All
thread changes should be done through (software) interrupts (e.g. calling
``thread_switch``).

System Calls in KUDOS
---------------------

KUDOS userland has a wrapper function for the ``syscall`` instruction, so there
is no need for the user to write code in assembly. In addition, some syscall
wrappers, with proper handling of syscall argumets are implemented in
``userland/lib.c``.  These wrappers, or rather, library functions, are
described below.

When implementing the system calls, the interface must remain *binary
compatible* with the unaltered KUDOS. This means that the already existing
system call function numbers must not be changed and that return value and
argument semantics are exactly as described below. When adding system calls not
mentioned below the arguments and return value semantics can of course be
defined as desired.

Halting the Operating System
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``void syscall_halt(void)``

  This is the only system call already implemented in KUDOS. It will unmount
  all mounted filesystems and then power off the machine (YAMS will terminate).
  This system call is the *only* method for userland processes to cause the
  machine to halt.

Process Related
^^^^^^^^^^^^^^^

``int syscall_spawn(char const *filename, char const **argv)``

  * Create a new (child) user process which loads the file identified by
    ``filename`` and executes the code in it.

  * ``argv`` specifies the arguments passed to the ``main()`` function
    of the new process, *not* including the name of the process
    itself.

  * Returns the process ID of the new process, or a negative number on
    error.

``void syscall_exit(int retval)``

  * Terminate the running process with the exit code ``retval``.

  * This function halts the process and never returns.

  * ``retval`` must be positive, as a negative value indicates a system
    call error in ``syscall_join()`` (see next).

``int syscall_join(int pid)``

  * Wait until the child process identified by ``pid`` is finished
    (i.e. calls ``process_exit()``).

  * Returns the exit code of the child, i.e., the value the child passed
    to ``syscall_exit()`` (or returned from ``main()``).

  * Returns a negative value on error.


File-System Related
^^^^^^^^^^^^^^^^^^^

``int syscall_read(int filehandle, void *buffer, int length)``
  * Read at most ``length`` bytes from the file identified by
    ``filehandle`` into ``buffer``.
  * The read starts at the current file position, and the file
    position is advanced by the number of bytes actually read.
  * Returns the number of bytes actually read (e.g. ``0`` if the file
    position is at the end of file), or a negative value on error.
  * If the ``filehandle`` is 0, the read is done from ``stdin``
    (the console), which is always considered to be an open file.
  * Filehandles 1 and 2 cannot be read from, and attempt to do so will
    always return an error code.

``int syscall_write(int filehandle, const void *buffer, int length)``
  * Write ``length`` bytes from ``buffer`` to the open file
    identified by ``filehandle``.
  * Writing starts at the current file position, and the file
    position is advanced by the number of bytes actually written.
  * Returns the number of bytes actually written, or a negative
    value on error. (If the return value is less than ``length`` but
    ≥ 0, it means that some error occured but that the file was still
    partially written).
  * If the ``filehandle`` is 1, the write is done to ``stdout`` (the
    console), which is always considered to be an open file.
  * If the ``filehandle`` is 2, the write is done to ``stderr`` (
    typically, also the console), which is always considered to be an open
    file.
  * Filehandle 0 cannot be written to and attempt to do so will always
    return an error code.

``int syscall_open(const char *pathname)``
  * Open the file addressed by ``pathname`` for reading and writing.
  * Returns the file handle of the opened file (non-negative), or a negative
    value on error.
  * Never returns values 0, 1 or 2, because they are reserved for ``stdin``,
    ``stdout`` and ``stderr``.

``int syscall_close(int filehandle)``
  * Close the open file identified by ``filehandle``.
  * ``filehandle`` is no longer a valid file handle after this call.
  * Returns zero on success, other numbers indicate failure (e.g.
    ``filehandle`` is not open so it can't be closed).

``int syscall_create(const char *pathname, int size)``
  * Create a file addressed by ``pathname`` with an initial size of ``size``.
  * The initial size means that at least size bytes, starting from the
    beginning of the file, can be written to the file at any point in the
    future (as long as it is not deleted), i.e. the file is initially
    allocated ``size`` bytes of disk space.
  * Returns 0 on success, or a negative value on error.

``int syscall_delete(const char *pathname)``
  * Remove the file addressed by ``pathname`` from the filesystem it resides
    on.
  * Returns 0 on success, or a negative value on error.
  * Note that it is impossible to implement a clean solution for the delete
    interaction with open files at the system call level. You are not
    expected to do that at this time (filesystem chapter has a separate
    exercise for this particular issue).

``int syscall_seek(int filehandle, int offset)``
  * Set the file position of the open file identified by ``filehandle`` to
    ``offset``.
  * Returns 0 on success, or a negative value on error.

``int syscall_filecount(const char *pathname)``
  * Get the number of files in a directory.
  * Returns 0 on success, or a negative value on error.

``int syscall_file(const char *pathname, int nth, char *buffer)``
  * Put the name of the ``nth`` file in the directory addressed by
    ``pathname`` into ``buffer``.
  * Returns 0 on success, or a negative value on error.

Exercises
---------

1. ⌨ Implement a new system call ``syscall_hello`` in KUDOS with the system
   call number ``0xAAA``. As a result of issuing the system call, KUDOS should
   print "Hello, World!" to the terminal and return to the user.

   You will need to define this system call number in ``kudos/proc/syscall.h``,
   handle it in ``kudos/proc/syscall.c``, define a wrapper for it in
   ``userland/lib.h``, and write the wrapper itself in ``userland/lib.c``. Last,
   but not least, write a userland program ``userland/hello.c`` (similar to
   ``userland/halt.c``) to test it.

   You can use either the polling TTY, or the interrupt-driven TTY :doc:`device
   driver <builtin-drivers>`.

