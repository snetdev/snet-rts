#!/bin/sh
#
# Test the distributed runtime systems for a range of different test applications.
#

TESTS="singlebox serial parallel split star locsplit feedback blinkstar nesteddynamic nestedstar innerroot"
THREADS="front pthread lpel"

function show () {
  echo
  echo "######################################################################"
  echo "### $@"
  echo
}

function tell () {
  echo
  echo "### $@"
  echo "######################################################################"
  echo
}

function warn () {
  echo "$0: $@" >&2
}

function fail () {
  warn "$@"
  exit 1
}

function build () {
  case "$thread" in
  front) ext="" ;;
  pthread) ext=".pth" ;;
  lpel) ext=".lpel" ;;
  esac
  exe=$test$ext
  show "Building $exe"
  make $exe
}

function run () {
  show "Running $exe"
  mpirun -v -np $(<nodes) --hostfile hostfile ./$exe -i input.xml -w 1 2>&1 
  if [ $? = 0 ]; then
    tell "Test $exe successful"
  else
    tell "Test $exe failed!"
  fi
}

# Move to the directory with the test applications
case "$0" in
*/*) cd ${0%/*} || exit 1 ;;
*) ;;
esac

# Remember the current directory
TOPLEVEL=$PWD

for test in $TESTS
do
  cd $TOPLEVEL || exit 1
  if [ ! -d $test ]; then
    warn "Missing test directory $test"
  else
    cd $test || exit 1
    for thread in $THREADS
    do
      if build
      then
        run "exe=$exe"
      fi
    done
  fi
    
done

