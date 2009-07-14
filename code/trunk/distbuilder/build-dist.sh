#$Id$
#
# trivial distribution builder
#
# v0.01 initial version (fpz, 13.07.09)
#

# where to build the distribution
DIST=snet-dist
TMPDIR=/tmp
DESTDIR=$TMPDIR/$DIST

# what to delete from tree
DEL_FROM_ROOT="Makefile config* ide src tests TODO distbuilder"
EXAMPLE_DIR=$DESTDIR/examples
DEL_FROM_EXAMPLES="crypto factorial_* matrix socrad"

# misc
VERSION=`svn info $SNETBASE | grep Revision | awk '{print $2}'`
OS=`uname -s`
ARCH=`uname -p`

if [ -d $DESTDIR ]; then
  echo "Directory $DESTDIR exists, please delete.";
else
  echo "Exporting $SNETBASE...";
  svn export $SNETBASE $DESTDIR;
  echo "done"
  echo "Copying README"
  cp $SNETBASE/distbuilder/README.binary-distribution $DESTDIR/README
  echo "done"
  echo "Building Compiler & Runtime...";
  cd $DESTDIR;
  SNETBASE=$DESTDIR ./configure > /dev/null
  SNETBASE=$DESTDIR make prod > /dev/null
  echo "Symlinking executables/libraries..."
  ln -s $DESTDIR/bin/snetc.prod $DESTDIR/bin/snetc
  ln -s $DESTDIR/lib/libsnet.prod.a $DESTDIR/lib/libsnet.a
  ln -s $DESTDIR/lib/libsnetutil.prod.so $DESTDIR/lib/libsnetutil.so
  echo "Building C & SAC interfaces";
  cd $DESTDIR/interfaces/C > /dev/null
  SNETBASE=$DESTDIR make
  echo "SKIPPING SAC4C INTERFACE"
#  cd $DISTDIR/interfaces/SAC;
#  SNETBASE=$DESTDIR make
  echo "Saving config.mkf"
  cp $DESTDIR/src/makefiles/config.mkf $DESTDIR/save.config.mkf
  echo "Purging developer related contents..."
  for F in $DEL_FROM_ROOT; do
    echo "Deleting $DESTDIR/$F"
    rm -rf $DESTDIR/$F
  done
  echo "Purging unwanted examples..."
  for F in $DEL_FROM_EXAMPLES; do
    echo "Deleting $EXAMPLE_DIR/$F"
    rm -rf $EXAMPLE_DIR/$F
  done
  echo "Restoring config.mkf into original destination";
  mkdir $DESTDIR/src
  mkdir $DESTDIR/src/makefiles
  mv $DESTDIR/save.config.mkf $DESTDIR/src/makefiles/config.mkf
  echo "Building archive..."
  cd /tmp;
  tar cvfz $SNETBASE/snet-v$VERSION-$OS-$ARCH.tar.gz -C $TMPDIR $DIST > /dev/null
fi
