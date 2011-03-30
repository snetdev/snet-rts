#!/bin/bash

NUM_RECORDS=1000

rm -f x
for i in 100 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000
do
  echo $i >> x
  ./geninput.sh $NUM_RECORDS $i  > input-d$i.xml
done
