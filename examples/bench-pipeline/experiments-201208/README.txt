Comparative snet/lpel performance analysis
==========================================

:Authors: kena, merijn
:Date: July/Aug 2012

Configurations
==============

From merijn:

  LPEL
  adb51404ec7b - a0150f29067
  adb51404ec7b - 4c0fefa2a604 (task respawn)
  adb51404ec7b - b7c9133af604 (task respawn + free list)
  077434f5de12 - 4e2bb43c7cd5 (stefan's implementation) 
  077434f5de12 - ea61d2977378 (merijn's cleanup) w/ or w/o SCHEDULER_CONCURRENT_PLACEMENT

  pthread
  8b41d9a55899 - snetc 3575
  26173631db42 - snetc 3575 (task respawn)

The hashes refer to merijn's Hg repo (uses hg-git). The corresponding
git commits are tagged using the hg hashes in
https://github.com/knz/snet-rts and https://github.com/knz/lpel.

Installation / setup
====================

We use the "cassandra" server: AMD Opteron 16-core on 4 sockets @
2.3GHz, running CentOS.

Using "deploy.mk" we install multiple versions of SNet/LPEL
side-by-side::

  make -f deploy.mk -j8
  make -f deploy.mk TAG=pthread-base SNET_RTS_VERSION=hg-8b41d9a55899 SNETC_VERSION=3575 LPEL_VERSION=hg-a0150f29067 -j8
  make -f deploy.mk TAG=pthread-task-respawn SNET_RTS_VERSION=hg-26173631db42 SNETC_VERSION=3575 LPEL_VERSION=hg-a0150f29067 -j8
  make -f deploy.mk TAG=lpel-task-respawn SNET_RTS_VERSION=hg-adb51404ec7b LPEL_VERSION=hg-4c0fefa2a604 -j8
  make -f deploy.mk TAG=lpel-task-respawn-freelist SNET_RTS_VERSION=hg-adb51404ec7b LPEL_VERSION=hg-b7c9133af604  -j8
  make -f deploy.mk TAG=lpel-stefan-migration SNET_RTS_VERSION=hg-077434f5de12 LPEL_VERSION=hg-4e2bb43c7cd5 -j8
  make -f deploy.mk TAG=lpel-stefan-migration-waiting SNET_RTS_VERSION=hg-077434f5de12 LPEL_VERSION=hg-4e2bb43c7cd5 CPPFLAGS_VARIANT=w -j8
  make -f deploy.mk TAG=lpel-cleanup-20120816 SNET_RTS_VERSION=hg-077434f5de12 LPEL_VERSION=hg-ea61d2977378 -j8
  make -f deploy.mk TAG=lpel-cleanup-20120816-waiting SNET_RTS_VERSION=hg-077434f5de12 LPEL_VERSION=hg-ea61d2977378 CPPFLAGS_VARIANT=w -j8
  make -f deploy.mk TAG=lpel-cleanup-20120816-concsched SNET_RTS_VERSION=hg-077434f5de12 LPEL_VERSION=hg-ea61d2977378 CPPFLAGS_VARIANT=cs -j8
  make -f deploy.mk TAG=lpel-cleanup-20120816-waiting-concsched SNET_RTS_VERSION=hg-077434f5de12 LPEL_VERSION=hg-ea61d2977378 CPPFLAGS_VARIANT=wcs -j8

This uses the git tags on https://github.com/knz/snet-rts and https://github.com/knz/lpel, as documented above.

As per ``deploy.mk`` the installation goes to ``/scratch/opt/snet``

Benchmark execution
===================

We automate the execution as follows.

1. the list of "interesting" combinations is put into the file ``configs``
2. the following command is used to generate a shell script ``cmds``::
  
     python gentests.py configs /scratch/opt/snet c >cmds

3. the file ``cmds`` is executed.

The result is a batch of files with names of the form ``result-*``,
each containing 3 timings.

Result gathering & plotting
===========================

We automate the gathering as follows.

1. the list of "interesting" plots is put into the file ``combis``
2. the following command is used to generate data files (``*.dat``) and gnuplot scripts (``*.gp``)::
  
     python gentests.py configs /scratch/opt/snet g combis

3. gnuplot is executed over each ``*.gp`` file in turn.





