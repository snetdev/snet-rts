#$Id$
#
# trivial distribution builder
#
# v0.01 initial version (fpz, 13.07.09)
# v0.02 minor refinements 
#

# where to build the distribution
DIST=snet-dist
TMPDIR=/tmp
DESTDIR=$TMPDIR/$DIST

# what to delete from tree
DEL_FROM_ROOT="Makefile config* README.MPI ide src tests TODO distbuilder"
EXAMPLE_DIR=$DESTDIR/examples
DEL_FROM_EXAMPLES="mti-stap factorial_mpi matrix ocrad raytracing"

# misc
VERSION=`svn info $SNETBASE | grep Revision | awk '{print $2}'`
OS=`uname -s`
ARCH=`uname -p`

GMCHECK=`which gmake`
if [ $? -ne 0 ];
then
MAKE=make
else
MAKE=gmake
fi


if [ -d $DESTDIR ]; then
  echo "Directory $DESTDIR exists, please delete.";
else
  echo "Exporting $SNETBASE...";
  svn export $SNETBASE $DESTDIR;
  echo "done"
  echo
  echo "Copying README"
  cp $SNETBASE/distbuilder/README.binary-distribution $DESTDIR/README
  echo "done"
  echo
  echo "Building Compiler & Runtime (ignore svn warnings)...";
  cd $DESTDIR;
  SNETBASE=$DESTDIR ./configure > /dev/null
  SNETBASE=$DESTDIR $MAKE prod > /dev/null
  echo "done"
  echo
  echo "Renaming/copying executables/libraries..."
  mv $DESTDIR/bin/snetc.prod $DESTDIR/bin/snetc
  cp $DESTDIR/lib/libsnet.prod.so $DESTDIR/lib/libsnet.so
  cp $DESTDIR/lib/libsnetutil.prod.so $DESTDIR/lib/libsnetutil.so
  echo "done"
  echo
  echo "Building C & SAC interfaces";
  cd $DESTDIR/interfaces/C 
  SNETBASE=$DESTDIR $MAKE > /dev/null
  echo "SKIPPING SAC4C INTERFACE"
  echo "done"
  echo
#  cd $DISTDIR/interfaces/SAC;
#  SNETBASE=$DESTDIR $MAKE
  echo "Saving config.mkf"
  cp $DESTDIR/src/makefiles/config.mkf $DESTDIR/save.config.mkf
  echo "done"
  echo
  echo "Purging developer related contents..."
  for F in $DEL_FROM_ROOT; do
    echo " - Deleting $DESTDIR/$F"
    rm -rf $DESTDIR/$F
  done
  echo done
  echo
  echo "Purging unwanted examples..."
  for F in $DEL_FROM_EXAMPLES; do
    echo " - Deleting $EXAMPLE_DIR/$F"
    rm -rf $EXAMPLE_DIR/$F
  done
  echo "done"
  echo
  echo "Restoring config.mkf into original destination";
  mkdir $DESTDIR/src
  mkdir $DESTDIR/src/makefiles
  mv $DESTDIR/save.config.mkf $DESTDIR/src/makefiles/config.mkf
  echo "done"
  echo
  echo "Building archive..."
  cd /tmp;
  tar cvf $SNETBASE/snet-v$VERSION-$OS-$ARCH.tar -C $TMPDIR $DIST > /dev/null
  gzip $SNETBASE/snet-v$VERSION-$OS-$ARCH.tar
  echo
  echo "Done."
  echo "Archive saved to: $SNETBASE/snet-v$VERSION-$OS-$ARCH.tar.gz"
  echo
fi
