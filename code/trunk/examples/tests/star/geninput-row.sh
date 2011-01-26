#!/bin/bash

NUM_RECORDS=1000

for i in 10 50 100 500 1000 5000 10000 50000
do
  ./geninput.sh $NUM_RECORDS $i  > input-d$i.xml
done
