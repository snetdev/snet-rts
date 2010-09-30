#!/bin/bash

#$Id$
#
# trivial distribution builder
#
# v0.01 initial version (fpz, 13.07.09)
# v0.02 minor refinements 
# v0.03 compilation now based on svn tree (Sep.10)
#

# where to build the distribution
DIST=snet
TMPDIR=`mktemp -d -t snetbuild.XXXXX`
DESTDIR=$TMPDIR/$DIST
CDIR=$TMPDIR/snetsrc

# what to copy from tree
CPY_FROM_ROOT="LICENSE bin interfaces lib include"
EXAMPLE_DIR=$DESTDIR/examples
CPY_FROM_EXAMPLES="crypto des factorial mandelbrot mini-C mini-SAC sudoku"

# misc
URL="svn+ssh://svn@obelix.stca.herts.ac.uk/home/svn/repositories/snet/code/trunk"
VERSION=`svn info $URL | grep Revision | awk '{print $2}'`
OS=`uname -s`
ARCH=`uname -p`
DISTFILE=$HOME/snet-v$VERSION-$OS-$ARCH


GMCHECK=`which gmake`
if [ $? -ne 0 ];
then
MAKE=make
else
MAKE=gmake
fi


if [ -f $DISTFILE* ]; then
  echo "$DISTFILE* exists, please delete.";
else
  echo;
  echo "========================================================="
  echo " S-Net Distribution Builder" 
  echo "---------------------------------------------------------"
  echo 
  echo " - TmpDir: $TMPDIR"
  echo " - Final Archive will be: $DISTFILE.tar.gz"
  echo
  echo "========================================================="
  echo
  mkdir $DESTDIR;
  echo "Checking out sources...";
  svn co -q $URL $CDIR;
  echo "done";
  echo;

  echo "Configuring sources..."; 
  cd $CDIR;
  SNETBASE=$CDIR ./configure > /dev/null;
  echo "done";
  echo;

  echo "Building Compiler & Runtime...";
  NDSFLAGS=`cat $CDIR/src/makefiles/config.mkf | grep -m 1 CCFLAGS | awk -F':=' '{print $2}' | sed 's/-g//g'`;
  cd $CDIR; $MAKE CCFLAGS="$NDSFLAGS" > /dev/null;
  cd $CDIR/interfaces/C; SNETBASE=$CDIR $MAKE > /dev/null; 
  echo "done";
  echo;

  echo "Constructing Distribution...";
  rm -rf `find $CDIR -type d -name ".svn"`
  rm -rf `find $CDIR -name "*.o"`
  for f in $CPY_FROM_ROOT; do
    cp -r $CDIR/$f $DESTDIR
  done;
  mkdir $DESTDIR/examples
  for f in $CPY_FROM_EXAMPLES; do
    cp -r $CDIR/examples/$f $DESTDIR/examples/
  done;
  mkdir -p $DESTDIR/src/makefiles;
  cp $CDIR/src/makefiles/config.mkf $DESTDIR/src/makefiles;
  cp $CDIR/distbuilder/README.binary-distribution $DESTDIR/README;
  echo "done";
  echo;

  echo "Building Distribution Archive...";
  tar -C $DESTDIR/.. -cf - $DIST | gzip -c > $DISTFILE.tar.gz;
  echo "done";

  echo
  echo "Cleaning up ..."
  rm -rf $TMPDIR
  echo "All done ($DISTFILE.tar.gz created)."
  echo

fi # check for file 


