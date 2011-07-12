#!/bin/bash

DEPTH=5

rm -f x
for i in 10 50 100 150 200 250 300 400 500
do
  echo $i >> x
  ./geninput.sh $i $DEPTH  > input-d$i.xml
done
