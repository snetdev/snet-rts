#!/bin/bash

NUM_RECORDS=${1:-100}
MAX_REP_DEPTH=${2:-10}

function header () {
  echo '<?xml version="1.0" encoding="ISO-8859-1" ?>'
  echo
}

function trailer () {
  echo '<record type="terminate" />'
}

function record () {
cat <<END
<record xmlns="snet-home.org" type="data" mode="textual" interface="C4SNet">
  <field label="A" >(int)$1</field>
  <field label="B" >(int)$1</field>
  <field label="D" >(int)$2</field>
</record>

END
}

header
for (( i=1 ; i <= $NUM_RECORDS ; i++ )) 
do
  ran=`expr $RANDOM % ${MAX_REP_DEPTH}`
  record $i $ran
done
trailer

