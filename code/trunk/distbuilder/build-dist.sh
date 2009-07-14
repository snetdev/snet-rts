#$Id$
#
# trivial distribution builder
#
# v0.01 initial version (fpz, 13.07.09)
#

DESTDIR=/tmp/snet-dist

DEL_FROM_ROOT="Makefile config* ide src tests TODO distbuilder"

EXAMPLE_DIR=$DESTDIR/examples
DEL_FROM_EXAMPLES="crypto factorial_* matrix socrad"

if [ -d $DESTDIR ]; then
  echo "Directory $DESTDIR exists, please delete.";
else
  echo "Exporting $SNETBASE...";
  svn export $SNETBASE $DESTDIR;
  echo "done"
  echo "Copying README"
  cp $SNETBASE/distbuilder/README.binary-distribution $DESTDIR
  echo "done"
  echo "Building Compiler & Runtime...";
  cd $DESTDIR;
  SNETBASE=$DESTDIR ./configure
  SNETBASE=$DESTDIR make
  echo "Building C & SAC interfaces";
  cd $DISTDIR/interfaces/C;
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

fi
