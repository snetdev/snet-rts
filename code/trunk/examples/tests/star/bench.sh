#!/bin/bash

rm -f bench.$1.tab.out

echo -n Benchmarking
for i in 10 50 100 500 1000 5000 10000
do
  echo -n " $i"
  /usr/bin/time -f "time %e real, %U user, %S sys, VCS %w, FCS %c, page-faults [%F,%R]" ./$1 -w 1 -m 4 < input-d$i.xml > /dev/null 2>> bench.$1.tab.out
  #cat bench.$1.$i.out >> bench.$1.tab.out
done
echo

