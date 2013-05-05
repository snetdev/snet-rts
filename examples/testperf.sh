#!/bin/bash --norc
#
# Test various example applications and show timings.
#
# Usage: testperf.sh [ -c | -r | -t threading ]
#

# The applications to be tested
APPLICATIONS="fibonacci poweroftwo sieve loadbalance"
APPLICATIONS+=" cannons-algorithm matrix jpegCode"
APPLICATIONS+=" cholesky crypto"

# Which threading layers to test
THREAD="pthread lpel lpel_hrc front"

# Check options: -c for compile, -r for run, -t for threading.
com=
run=
thr=
while [ $# -gt 0 ]
do
    case "$1" in
        -c) com=1 ;;
        -r) run=1 ;;
        -t) shift ; thr+=" $1" ;;
        pthread|lpel|lpel_hrc|front) thr+=" $1" ;;
        *) echo "$0: Unknown option: $1"  >&2
           echo "Usage: $0 [ -c | -r | -t threading ... ]" >&2; exit 1 ;;
    esac
    shift
done

# If no options were given, then set them all.
if [ -z "$com" ] && [ -z "$run" ]
then
    com=1
    run=1
fi
if [ -z "$thr" ]
then
    thr=$THREAD
fi

# Compile all executables for all threadings
if [ "$com" = 1 ]
then
    for d in $APPLICATIONS
    do
        echo
        echo "######################################################################"
        echo "### make in $d for '$thr'"
        echo
        make -s -C $d clean everything >/dev/null || exit 1
        make -s -C $d all $thr >/dev/null || exit 1
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
        make -s -C $d test
    done
fi

