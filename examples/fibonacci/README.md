Generating Fibonacci Numbers in Pure S-Net
==========================================

The programs in this directory provide a crude way to measure
the efficiency and scalability of the S-Net runtime system.
They all all compute Fibonacci Numbers without the use of boxes.
They differ in the way they accomplish this goal.

Fibonacci.snet
--------------

Computes:

    fib(n) = ((n <= 1) ? 1 : (fib(n-1) + fib(n-2)))

as precisely as possible. That is, each sub-expression
is computed and added in the way the computation unfolds.
The tag <M> holds a mask which uniquely identifies a certain subexpression.
Only <V> and <W> records with an identical mask are added.

Fibonacci2.snet
---------------

Computes Fibonacci Numbers as efficiently as possible in pur S-Net.

Fibonacci3.snet
---------------

Is a more efficient variation on Fibonacci.snet.
This relaxes the constraints on which operands can be added.
To this end a recursion level <L> is computed as well.
<V> and <W> records with an identical <L> can be added.

This is the version which was used in thesis GijsbersMSc13.

Fibonacci5.snet
---------------

This is a version of Fibonacci.snet where all combinators
are deterministic. This allows to measure the influence
of determinism compared to Fibonacci.snet.

Fibonacci7.snet
---------------

This is a version of Fibonacci3.snet where all combinators
are deterministic. This allows to measure the influence
of determinism compared to Fibonacci3.snet.

