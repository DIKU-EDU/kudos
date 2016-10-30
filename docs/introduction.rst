Introduction
============

The KUDOS operating system is heavily based upon the BUENOS operating
system, and this documentation is heavily based upon the accompanying
"BUENOS Roadmap".

BUENOS was originally developed at `Aalto University
<https://www.niksula.hut.fi/>`_, Finland.  KUDOS is a continuation of the
BUENOS effort at the `Department of Computer Science at the University of
Copenhagen (DIKU) <http://www.diku.dk/>`_, Denmark. For more information about
BUENOS visit the project homepage at: http://www.niksula.hut.fi/u/buenos/.

The KUDOS system supports multiple CPUs, provides threading and a wide
variety of synchronization primitives. It also includes skeleton code for
userland program support, partial support for a virtual memory subsystem, a
trivial filesystem, and generic drivers for textual input and output.

Currently, KUDOS can run on top of YAMS, Yet Another MIPS Simulator,
originally developed alongside BUENOS, or on an x86-64 simulator like QEMU;
the latter will be the focus of this manual. All that you
need to know, is that KUDOS is, at least in principle, a cross-platform
operating system.

The main idea of KUDOS is to give you a real, working multiprocessor
operating system kernel which is as small and simple as possible. KUDOS
could be quite easily ported to other architectures; only device drivers and
boot code need to be modified.  A virtual machine environment is used because
of easier development, static hardware settings and device driver simplicity,
not because unrealistic assumptions are needed by the kernel.

If you are a student participating in an operating systems project
course, the course staff has probably already set up a development
environment for you. If they have not, you must acquire YAMS (see
below for details) and compile it. You also need a MIPS32 ELF cross
compiler to compile KUDOS for use with YAMS.

Expected Background Knowledge
-----------------------------

Since the KUDOS system is written using the C programming language, you
should be able to program in C. For an introduction to the C programming
language, see the classical reference [KR]_, or the more modern, and perhaps
more accessible, [ModernC]_.

We also expect that you are taking a course on operating systems or otherwise
know the basics about operating systems. You can still find OS textbooks very
handy when doing the exercises. We recommend that you get a hold of the book
"Operating Systems: Three Easy Pieces"[OSTEP]_, if you are in a more
classical mood, [Stallings]_ or [Tanenbaum]_, or the more system approach found in [BOH]_.

Since you are going to interact directly with the hardware quite a
lot, you should know something about hardware. A classical introduction on
this can be found in the book [COD5e]_, while [BOH]_ again gives a complete perspective of computing systems.

Since kernel programming generally involves a lot of synchronization issues, a
course on concurrent programming is recommended to be taken later on. One good
book in this field is [Andrews]_. These issues are also handled in the
operating systems books cited above, but the approach is different.

How to Use This Documentation
-----------------------------

This documentation is designed to be used both as read-through introduction and
as a reference guide. To get most out of this document you should probably:

1. Read :ref:`usage` and \autoref{sec:overview} (system
   overview) carefully.

2. Skim through the whole document to get a good overview.

3. Before designing and implementing your assignments, carefully read all
   chapters on the subject matter.

4. Use the document as a reference when designing and implementing your
   improvements.

.. 
   KUDOS for teachers
   ----------------------

   As stated above, the KUDOS system is meant as an assignment backbone for
   operating systems project courses. This document, while primarily acting as
   reference guide to the system, is also designed to support project courses.
   The document is ordered such that various kernel programming issues are
   introduced in sensible order and exercises (see also exercises_) are
   provided for each subject area.

   While the system as such can be used as a base for a large variety of
   assignments, this document works best if assignments are
   divided into four different parts as follows:

   1. **Synchronization and Multiprogramming**. Various multiprogramming issues
      relevant on both multiprocessor and uniprocessor machines are covered in
      \autoref{sec:threading} and \autoref{sec:sync}.

   2. **Userland**. Userland processes, interactions between
      kernel and userland as well as system calls are covered in
      \autoref{sec:userland}.

   3. **Virtual Memory**. The current virtual memory support
      mechanisms in KUDOS are explained in \autoref{sec:vm}, which also
      gives exercises on the subject area.

   4. **Filesystem**. Filesystem issues are covered in
      \autoref{sec:fs}.

   Preparing for a KUDOS Course
   --------------------------------
   ********************************

   To implement an operating systems project course with KUDOS, at least the
   following steps are necessary:

   * Provide students with a development environment with precompiled
   YAMS/QEMU and a (MIPS32) ELF cross compiler. See specific usage guide for
   instructions on setup of simulator and the cross compiler environment.

   * Decide which exercises are used on the course, how many points
   they are worth and what are the deadlines.

   * Decide any other practical issues (are design reviews compulsory
   for students, how many students there are per group, etc.)

   * Familiarize the staff with KUDOS and the simulator.

   * Introduce KUDOS to the students.

Exercises
---------
.. _exercises:

Each chapter in this document contains a set of exercises. Some of
these are meant as simple thought challenges and some as much more
demanding and larger programming exercises.

The thought exercises are meant for self study and they can be used to
check that the contents of the chapter were understood. The
programming exercises are meant to be possible assignments on
operating system project courses.

The exercises look like this:

1. This is a theoretical exercise.

2. ⌨ This is a programming task.

References
----------

.. [KR] Brian Kernighan and Dennis Ritchie. *The C Programming Language*, 2nd Edition. Prentice-Hall, 1988.

.. [ModernC]  Jens Gustedt. *Modern C*. Unpublished, 2015. Available for free from http://icube-icps.unistra.fr/index.php/File:ModernC.pdf.

.. [OSTEP] Remzi H. Arpaci-Dusseau and Andrea C. Arpaci-Dusseau. *Operating Systems: Three Easy Pieces*. Arpaci-Dusseau Books, 2015.Available for free from http://pages.cs.wisc.edu/~remzi/OSTEP/.

.. [Stallings] William Stallings. *Operating Systems: Internals and Design Principles*, 4th edition. Prentice-Hall, 2001.

.. [Tanenbaum] Andrew Tanenbaum. *Modern Operating Systems*, 2nd edition. Prentice-Hall, 2001.

.. [COD5e] David A. Patterson and John L. Hennessy. *Computer Organization and Design*, 5th edition. Elsevier, 2014.

.. [Andrews] Gregory R. Andrews., *Foundations of multithreaded, parallel and distributed programming*. Addison-Wesley Longman, 2000.

.. [BOH] Randal E. Bryant and David R. O’Hallaron, *Computer Systems: A Programmer’s Perspective*, Pearson, 2016.
