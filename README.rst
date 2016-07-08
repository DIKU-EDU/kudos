KUDOS
=====

KUDOS is a skeleton operating system for exploring operating systems
concepts. It is intended for:

1. teaching operating system concepts, and
2. to serve as a baseline for open-ended student projects.

For more in-depth documentation, see |docs|.

.. |docs| image:: https://readthedocs.org/projects/kudos/badge/?version=latest
    :alt: Documentation Status
    :scale: 100%
    :target: https://kudos.readthedocs.org/en/latest/?badge=latest

Important Student Notice
------------------------

**When using KUDOS for course work, don't publicly fork this repository, or
otherwise make your course work public before you have passed the course.**

The policy at the University of Copenhagen is that a student that keeps their
course work public, is an accomplice to plagiarism, should any other student
choose to copy their work.

You are of course, completely safe to `clone`_ this repository, and keep a
local, working copy, `pulling`_ this remote as the course progresses.

.. _clone: https://help.github.com/articles/importing-a-git-repository-using-the-command-line/
.. _pulling: https://help.github.com/articles/fetching-a-remote/

KUDOS Notes
===========
KUDOS can only handle one device per ATA/IDE bus (master/slave don't work)
    - Workaround is to start KUDOS from a disk on one bus and start the initial program from
        a disk on another bus. (The IDE driver is generally fucked-up!)
    - KUDOS x86_64 cannot halt
