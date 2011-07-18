#!/bin/bash

echo '<?xml version="1.0" ?>'

NUM_REPEATS=${1:-1}
DEPTH=${2:-2}

for (( outer=0; outer<$NUM_REPEATS; outer++ ))
do
  # create A
  for (( inner=0; inner<$DEPTH; inner++ ))
  do
    NUM=`expr $outer \* $DEPTH + $inner`

    echo '<record type="data" mode="textual" >'
    echo  '  <field label="A" interface="C4SNet">(int)'$NUM'</field>'
    echo '</record>'
  done
  # create B
  for (( inner=0; inner<$DEPTH; inner++ ))
  do
    NUM=`expr $outer \* $DEPTH + $inner`
    echo '<record type="data" mode="textual" >'
    echo  '  <field label="B" interface="C4SNet">(int)'$NUM'</field>'
    echo '</record>'
  done
done

echo '<record type="terminate" />'
