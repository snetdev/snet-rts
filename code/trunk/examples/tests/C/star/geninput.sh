#!/bin/bash

echo '<?xml version="1.0" ?>'

for i in `seq 1 $1`
do
  NUM=`expr $RANDOM % $2`
  echo '<record xmlns="snet-home.org" type="data" mode="textual" >'
  echo '  <field label="A" interface="C4SNet">(int)'$NUM'</field>'
  echo '</record>'
done

echo '<record type="terminate" />'
