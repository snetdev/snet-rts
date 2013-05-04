#!/bin/bash --norc
#
# Test various example applications and show timings.
#
# Usage: testperf.sh [ -c | -r ]
#

# The applications to be tested
APPLICATIONS="fibonacci poweroftwo sieve loadbalance"
APPLICATIONS+=" cannons-algorithm matrix jpegCode"
APPLICATIONS+=" cholesky"

# Which threading layers to test
THREAD="pthread lpel lpel_hrc front"

# Check options -c for compile and -r for run.
com=
run=
for f
do
    case "$f" in
        -c) com=1 ;;
        -r) run=1 ;;
        *) echo "Usage: $0 [ -c | -r ]" ; exit 1 ;;
    esac
done

# If no options were given, then set them all.
if [ -z "$com" ] && [ -z "$run" ]
then
    com=1
    run=1
fi

# Compile all executables for all threadings
if [ "$com" = 1 ]
then
    for d in $APPLICATIONS
    do
        echo
        echo "######################################################################"
        echo "### make in $d "
        echo
        make -C $d clean everything >/dev/null || exit 1
        make -C $d all $THREAD >/dev/null || exit 1
    done
fi

# Run the applications for all compiled threadings
if [ "$run" = 1 ]
then
    for d in $APPLICATIONS
    do
        echo
        echo "######################################################################"
        echo "### run in $d "
        echo
        make -C $d test
    done
fi

