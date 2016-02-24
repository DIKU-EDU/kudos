Userland Processes
==================

KUDOS has currently implemented a very simple support for processes
run in userland. Basically processes differ from threads in that they
have an individual virtual memory address space. Userland processes
won't of course have an access to kernel code except via system
calls. There is currently no separate process table.

Processes are started as regular kernel threads. During process
startup in the function ``process_start()``, function
``thread_go_to_userland()`` is called. This function will switch the
thread to user mode by setting the user mode bit in the CP0 status
register. After this, a context switch is done. Next time the thread
is switched to running mode it will run in user mode.  It is
critically important that you understand this design: a "process" is
really just a name for a kernel thread that executes code in user
mode.  Not every kernel thread supports a process, but every process
is associated with a kernel thread.  A syscall makes the kernel thread
switch from user mode to kernel mode, but it is still the same kernel
thread that runs.

Processes have their own virtual memory address space. In the case of
user processes this space is limited to *user mapped* segment of the
virtual memory address space. Individual virtual memory space is
provided by creating a pagetable for the process. This is done by
calling ``vm_create_pagetable()``. Because of the limitations of the
current virtual memory system, the whole pagetable must fit to the TLB
at once. This limits the memory space to 16 pages (16 * 4096
bytes). Both the userland binary and the memory allocated for the data
must fit in this limited space.

Because processes are run in kernel threads, the ``thread_table_t``
structure has a few fields for (userland) processes. In context
switches ``user_context`` is set to point to the saved user context of
the process (register contents and other such state). The context
follows the regular ``context_t`` data structure. The ``pagetable`` field
is provided for the pagetable created during process startup. The
``process_id`` field is currently not used. It could be used for example
as an index to a separate process table.

Process Startup
---------------

New processes can currently be started by calling the function
``process_start()``. The function needs to be modified before used to
implement the ``spawn`` system call, but it can be used to fire up test
processes::

    void process_start(char const *executable, char const **argv)

* Starts one userland process. The code and data for the process is
  loaded from file \texttt{executable}.

* The thread calling this function will be used to run the process. A
  call to this function will never return.

*Implementation*:

1. Retrieve the thread ID of the running kernel thread.

2. Setup the running thread for running a process, using a call to
   ``setup_new_process()`` (described below).

3. Set the page table of the running kernel thread to the page table
   we created for the process, by calling ``process_set_pagetable()``.
   This function _must_ be called from the kernel thread that will run
   the process - for now, this is the same thread that calls
   ``process_start()``.

4. Setup the user context - this involves setting every register to
   zero, except for the instruction pointer and the stack pointer,
   which will be set to their proper values (determined by the call to
   ``setup_new_process()``).

5. Go to user mode using the just created user context.

Most of the interesting work is done by the ``setup_new_process()``
function.  You do not need to understand the details of its
implementation (although it is well commented and not particularly
complicated, except for some address arithmetic).  You will however
need to understand its interface::

    int setup_new_process(TID_t thread,
                          const char *executable,
                          const char **argv_src,
                          virtaddr_t *entry_point, virtaddr_t *stack_top)


* Prepares the given kernel thread (specified by thread ID) for
  executing a user process.

* Loads a program from the given file name - this must be a file
  containing an executable in ELF format.

* Memory will be pre-allocated for stack, static data, and code, as
  indicated in the executable.  A page table will be created and and
  stored in the ``pagetable`` field of the given thread.

* ``argv_src`` must be a NULL-terminated array of C strings.  These
  will also be copied into the memory of the process.  This is used to
  setup the ``argv`` array that will be passed to the ``main()``
  function of the new process.

* ``*entry_point`` will be set to the desired initial value of the
  instruction pointer.

* ``*stack_top`` will be set to the desired initial value of the stack
  pointer.

Exception Handling
------------------

When an exception occurs in user mode the context switch code switches
the current thread from user context to kernel context. The thread
will resume its execution in kernel mode in function
``user_exception_handle()``. This function will handle the TLB misses
and system calls caused by the userland process::

    void user_exception_handle(int exception)

* This function is called when an exception has occured in user
  mode. Handles the given ``exception``.

* Dispatches system calls to the syscall handler, panics the kernel on
  other exceptions.
