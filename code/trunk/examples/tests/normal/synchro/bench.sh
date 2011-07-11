#!/bin/bash


for w in 1 2 # 4 8 16 24 32 40 48
do
  FNAME=bench.$1-w$w.tab
  rm -f $FNAME
  echo -n Benchmarking w=$w
  for i in `cat x`
  do
    echo -n " $i"
    /usr/bin/time -f "time %e real, %U user, %S sys, VCS %w, FCS %c, page-faults [%F,%R]" ./$1 -w $w -m 0 < input-d$i.xml > /dev/null 2>> $FNAME
  done
  echo
done
