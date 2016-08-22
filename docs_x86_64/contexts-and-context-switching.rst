Contexts and Context Switching
==============================

A context is the state of some particular computational process, such as a
thread, and includes the CPU registers and stack. A context switch occurs when
threads are switched in and out, which must occur opaquely for the
computational process itself: in practice done by saving and restoring the
program counter and stack pointer registers. The context of a thread is saved
in the context t structure, which is usually referenced by a pointer in thread
t in the thread table (see section 3.1.1). Contexts are always stored in the
stack of the corresponding thread.

Context switching is traditionally the most bizarre piece of code in most
operating systems, as the context switch code must be written in assembler to
be able to store registers etc, and is thus very architecture specific.

Context Switching in ``kudos-mips32``
-------------------------------------

+--------------+--------------+------------------------------------+
| Type         | Field name   | Explanation                        |
+--------------+--------------+------------------------------------+
| ``uint32_t`` | ``cpu_regs`` | MIPS32 CPU registers,              |
|              |              | except ``zero``, ``k0`` and ``k1`` |
+--------------+--------------+------------------------------------+

