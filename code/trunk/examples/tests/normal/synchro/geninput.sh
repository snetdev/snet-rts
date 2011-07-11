#!/bin/bash

echo '<?xml version="1.0" ?>'

DEPTH=$2

for (( outer=0; outer<$1; outer++ ))
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
