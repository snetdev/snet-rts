=============================================
 Automated synthetic tests for the S-Net RTS
=============================================

:Author: kena
:Date: July 2012

Overview
========

This directory defines both a test suite and benchmark suite for the
S-Net run-time system.

The Makefile it contains can both:

- generate networks of different structures;
- compile each network using different RTS flags (eg different threading);
- run these networks over different run-time parameters.

Generated networks
==================

All generated networks share the same overall structure: 

- the network source file is ``testB_L_W.snet``;

- the outer network is a pipeline of L stages;

- each stage is composed of a parallel section of W branches;

- each branch contains a configurable "inner network" of type B.

If W is set to 0, then the parallel combinator is elided and the inner
network is directly repeated in the outer pipeline.

Different types B of inner networks are supported:

- B="pt": each inner network is a simple box, of configurable workload
- B="sn"/B="sd": each inner network is a star ("n" for non-deterministic, "d" for deterministic)
- B="bn"/B="bd": each inner network is a bling containing a simple box.

Compilation
===========

The Makefile contains rules to compile each network to binary files
with the name ``testB-L-W-T-D/prog`` where:

- T is the threading back-end (lpel/pthread)

- D is the distribution back-end (nodist/...)

Automated execution
===================

The Makefile contains rules to run each network over 4 ranged parameters:

- N: the number of input records generated;

- M: the amount of work per inner box;

- C: the degree of expansion for blings and stars;

- P: the number of cores to use (for back-ends that support it)

The command "make check" will perform the cross-product across some
pre-selected values for each parameter B, L, W, N, M, C, P, T, D.

To run other values simply request the file ``result-B-L-W-N-M-C-P-T-D``.

For example::

    make result-pt-2-3-4-5-6-7-lpel-nodist

will:

- run the network "test_pt_2_3", ie a pipeline of 2 stages, each stage containing 3 parallel branches, each containing a box;

- over 4 input records;

- each defining 5 units of work for each inside box;

- using 6 levels of expansion for inner blings and stars (unused in this example: "pt" does not have inner stars/blings);

- using 7 cores max;

- with the lpel back-end;

- and no distribution.

Tuning / environment variables
==============================

The following environment variables influence the execution:

``BENCH_RUNS``
   The number of times each program is run to produce a ``result`` file. Defaults to 3.

``BENCH_EXTRA_ARGS``
   Extra command-line arguments to pass to each program. Defaults to none.

``WORKLOAD_FUZZINESS``
   Fuzziness factor to apply to the N parameter. Defaults to 0%. When
   non-zero, must be a percentage. The effect is that the workload of
   each box will between [N*(1-F), N].

