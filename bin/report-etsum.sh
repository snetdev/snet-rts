#!/bin/bash

report-et.py $1 | grep "\*\*\*" \
  | cut -f 4 -d ' ' | cut -f 1 -d ',' \
  | awk '{sum+=$1} END { print "sum=", sum, "avg=",sum/NR}'

