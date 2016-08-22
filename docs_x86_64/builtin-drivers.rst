Built-in Drivers
================

KUDOS ships with several built-in working drivers.  The drivers for the MIPS
target are all designed to work with YAMS hardware.  The purpose of this section
is to show how one can create actual drivers for for the KUDOS device driver
interface.

Polling TTY Driver
------------------

Two separate drivers are provided for the TTY (the terminal).  The first one is
implemented by *polling* and the other with *interrupt handlers*.

Polling means that the kernel again and again asks the (virtual, in this case)
hardware if anything new has come up.  Depending on interrupt handlers means
that the hardware signals the kernel when a change has occured.

The polling driver is needed when booting up, since interrupts are disabled.  It
is also useful in kernel panic situations, because interrupt handlers might not
be relied on in such error cases.

Perhaps the easiest way to use the polling TTY driver is using the builtin
functions ``kwrite`` and ``kprintf`` (defined in ``kudos/lib/libc.h``). See
``kudos/drivers/polltty.h`` and ``kudos/drivers/$ARCH/polltty.c`` for the
implementation and documentation of the driver itself.

Interrupt-driven TTY Driver
---------------------------

The interrupt driven (i.e. the *asynchronous*) TTY driver is the terminal device
driver used most of the kernel terminal I/O-routines.  The terminal driver has
two functions to provide output to the terminal and input to the kernel.  Both
of these happen asynchronously. i.e.  the input handling is triggered when the
user presses a key on the keyboard.  The output handler is invoked when some
part of the kernel requests a write.  The asynchronous TTY driver is implemented
in ``drivers/$ARCH/tty.c`` and implements the **generic character device
interface**.

The following functions implement the TTY driver:

``device_t *tty_init(io_descriptor_t *desc)``

Initialize a driver for the TTY defined by ``desc``.  This function is called
once for each TTY driver present in the YAMS virtual machine.

Implementation:

  1. Allocate memory for one ``device_t``.
  2. Allocate memory for one ``gcd_t`` and set ``generic_device`` to point to
     it.
  3. Set ``gcd->device`` to point to the allocated ``device_t``, ``gcd->write``
     to ``tty_write`` and ``gcd->read`` to ``tty_read``.
  4. Register the interrupt handler (``tty_interrupt_handle``).
  5. Allocate a structure that has (small) read and write buffers and head and
     count variables for them, and a spinlock to synchronize access to the
     structure and ``real_device`` to point to it. The first tty driver's
     spinlock is shared with ``kprintf()`` (i.e.  the first tty device is shared
     with polling TTY driver).
  6. Return a pointer to the allocated ``device_t``.

``void tty_interrupt_handle(device_t *device)``

Handle interrupts concerning ``device``. This function is never called
directly from kernel code, instead it is invoked from interrupt handler.

Implementation if WIRQ (*write interrupt request*) is set:

  1. Acquire the driver spinlock.
  2. Issue the WIRQD into COMMAND (inhibits write interrupts).
  3. Issue the Reset WIRQ into COMMAND.
  4. While WBUSY is not set and there is data in the write buffer, Reset WIRQ
     and write a byte from the write buffer to DATA.
  5. Issue the WIRQE into COMMAND (enables write interrupts).
  6. If the buffer is empty, wake up the threads sleeping on the write buffer.
  7. Release the driver spinlock.

Implementation if RIRQ (*read interrupt request* is set:

  1. Acquire the driver spinlock.
  2. Issue the Reset RIRQ command to COMMAND. If this caused an error, panic
     (*serious* hardware failure).
  3. Read from DATA to the read buffer while RAVAIL is set. Read *all* available
     data, even if the read buffer becomes filled (because the driver expects us
     to do this).
  4. Release the driver spinlock.
  5. Wake up all threads sleeping on the read buffer.

``static int tty_write(gcd_t *gcd, void *buf, int len)``

Write ``len`` bytes from ``buf`` to the TTY specified by ``gcd``.

Implementation:

  1. Disable interrupts and acquire driver spinlock.
  2. As long as write buffer is not empty, sleep on it (release-reacquire for
     the spinlock).
  3. Fill the write buffer from ``buf``.
  4. If WBUSY is not set, write ``one`` byte to the DATA port.  (This is needed
     so that the write IRQ is raised. The interrupt handler will write the rest
     of the buffer.)
  5. If there is more than one byte of data to be written, release the spinlock
     and sleep on the write buffer.
  6. If there is more data in ``buf``, repeat from step 3.
  7. Release spinlock and restore interrupt state.
  8. Return the number of bytes written.

``static int tty_read(gcd_t *gcd, void *buf, int len)``

Read at least one and at most ``len`` bytes into ``buf`` from the TTY specified
by ``gcd``.

Implementation:

  1. Disable interrupts and acquire driver spinlock.
  2. While there is no data in the read buffer, sleep on it (release-reacquire
     for the spinlock).
  3. Read ``MIN(len, data-in-readbuf)`` bytes into ``buf`` from the read buffer.
  4. Release spinlock and restore interrupt state.
  5. Return the number of bytes read.


Disk Driver
-----------

The disk driver implements the Generic Block Device (GBD) interface.  The driver
is interrupt-driven and provides both synchronous (blocking) and asynchronous
(non-blocking) operating modes for request.  The driver has three main parts:

  * An initialization function, which is called in startup when a disk is found.
  * An interrupt handler.
  * Functions which implement the GBD interface (read, write and information
    inquiring).

The disk driver maintains a queue of pending requests.  The queue insertion is
handled in disk scheduler, which currently just inserts new requests at the end
of the queue.  This queue, as well as access to the disk device, is protected by
a spinlock.  The spinlock and queue are stored in driver's internal data.  The
internal data also contains a pointer to the currently served disk request.

The disk driver is implemented and documented in ``kudos/drivers/$ARCH/disk.c``.
Note how the fields modified by both the inquiring and interrupt-ready parts of
the driver are marked as ``volatile``, so that the compiler won't optimize
access to them (store them in registers and assume that value is valid later,
which would be a flawed approach because of interrupts, which can change the
values of the variables asynchronously).


Timer Driver
------------

The Timer driver allows to set timer interrupts at certain intervals.  The
``timer_set_ticks()`` C function works as a front-end for the
``_timer_set_ticks`` assembler function.  The C function takes anumber of
processor clock cycles after the timer interrupt is wanted to happen, and it
passes it to the assembler function that does all work.

A timer interrupt is caused by using ``CP0`` registers ``Count`` and
``Compare``.  The ``Count`` register contains the current cycle count, and the
``Compare`` register contains the cycle number where the timer interrupt is to
happen.  The assembler function simply adds the number of cycles to the current
cycle count and writes it to the ``Compare`` register.

The timer driver is implemented and documented in ``kudos/drivers/timer.c`` and
``kudos/drivers/$ARCH/_timer.S``.


Metadevice Drivers
------------------

"Metadevices" is a name for those devices documented in the YAMS documentation
as non-peripheral devices (the ``0x100`` series).  They don't really interface
to any specific device but rather to the system itself (the motherboard main
chipset, firmware or similar).  The metadevices and their drivers are very
simple, and they are as follows.

See ``kudos/drivers/metadev.h`` and ``kudos/drivers/$ARCH/metadev.c`` for the
implementation and description of the following metadevices.

Meminfo
~~~~~~~

The system memory information device provides information about the amount of
memory present in the system.

RTC
~~~

The Real Time Clock (RTC) device provides simulated real time data, such as
system uptime and clock speed.  It is a wrapper to the RTC device I/O ports.

Shutdown
~~~~~~~~

The (software) shutdown device is used to either halt the system by dropping to
the YAMS console (firmware console) or "poweroff" the system by exiting YAMS
completely.

CPU Status
~~~~~~~~~~

Each processor has its own status device.  These devices can be used to count
the number of CPUs on the system or to generate interrupts on any CPU.

Exercises
---------

1. Both ``kwrite`` and ``kprintf`` use the polling TTY driver. Why?
