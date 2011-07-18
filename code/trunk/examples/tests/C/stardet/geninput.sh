#!/bin/bash

echo '<?xml version="1.0" ?>'

NUM_RECORDS=${1:-10}
MAX_REP_DEPTH=${2:-10}

for i in `seq 1 ${NUM_RECORDS}`
do
  NUM=`expr $RANDOM % ${MAX_REP_DEPTH}`
  echo '<record xmlns="snet-home.org" type="data" mode="textual" >'
  echo '  <field label="A" interface="C4SNet">(int)'$NUM'</field>'
  echo '</record>'
done

echo '<record type="terminate" />'
