#!/bin/bash

cd inputs


OUT_LPEL=split-lpel.out
echo "Benching lpel"
rm -f $OUT_LPEL.tmp
for i in `seq 1 10`
do
  time ../mandelbrot_split.lpel -m 5 -dloc < split-test.xml > /dev/null 2>> $OUT_LPEL.tmp
done
mv $OUT_LPEL.tmp $OUT_LPEL

OUT_PTHR=split-pthr.out
echo "Benching pthr"
rm -f $OUT_PTHR.tmp
for i in `seq 1 10`
do
  time ../mandelbrot_split < split-test.xml > /dev/null 2>> $OUT_PTHR.tmp
done
mv $OUT_PTHR.tmp $OUT_PTHR

cd ..
