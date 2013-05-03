Compute Prime Numbers Using the Sieve of Eratosthenes
=====================================================

The programs in this directory provide a crude way to measure
the efficiency and scalability of the S-Net runtime system.
They implement the Sieve of Eratosthenes.
This tests the performance on a long pipeline of
stateful primality filters.

The programs expect a single input record with
two numeric values <N> and <M>.
They compute all primes larger than <N>
up to and including <M>.

sieve.snet
--------------

This program uses state modeling as documented in
"The Essence of Synchronisation in Asynchronous Data Flow".

sieve2.snet
---------------

This program uses state modeling based on the feedback combinator
as documented in theses/GijsbersMSc13.

The programs show that the latter form of stateful modeling
is much more efficient for Front and Pthreads.
