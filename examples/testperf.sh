#!/bin/bash --norc
#
# Test various example applications and show timings.
#

APPLICATIONS="fibonacci poweroftwo sieve loadbalance"
APPLICATIONS="$APPLICATIONS cannons-algorithm matrix jpegCode"

THREAD="pthread lpel lpel_hrc front"

# Make sure all executables are made
for d in $APPLICATIONS
do
    echo
    echo "######################################################################"
    echo "### make in $d "
    echo
    make -C $d clean everything >/dev/null
    make -C $d all $THREAD >/dev/null
done

# Run the applications all threadings
for d in $APPLICATIONS
do
    echo
    echo "######################################################################"
    echo "### run in $d "
    echo
    make -C $d test
done


