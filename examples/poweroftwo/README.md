Computing The Power of Two in Pure S-Net
========================================

The programs in this directory provide a crude way to measure
the efficiency and scalability of the S-Net runtime system.
They all compute the power of two for a single input integer without
the use of boxes.  They differ slightly in their innermost loop.

poweroftwo.snet
--------------

Computes:

    po2(n) = ((n <= 1) ? 1 : (po2(n-1) + po2(n-1)))

The innermost loop is star over a synchro-cell.
Each recursion level has its own such loop.

poweroftwo2.snet
---------------

The purpose of this program is to demonstrate
the negative performance impact for threaded-entity
runtime systems when the collector entity
has to handle a large number of incoming connections.
Each synchronization is unique as determined
by the value of the <M> tag, which is a bitmask.
