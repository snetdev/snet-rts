#!/bin/bash

NUM_RECORDS=${1:-10}
MAX_REP_DEPTH=${2:-2}

declare -a a

sum=0
for (( i=1 ; i <= $NUM_RECORDS ; i++ )) 
do
  ran=`expr $RANDOM % ${MAX_REP_DEPTH}`
  if [ $ran = 0 ] && [ $i = $NUM_RECORDS ]; then
    ran=1
  fi
  a+=($ran)
  let sum=$sum+$ran
done

cat <<END
<?xml version="1.0" encoding="ISO-8859-1" ?>

<!-- Initial state first -->
<record type="data" mode="textual" interface="C4SNet">
  <field label="state" >(int)$sum</field>
</record>

<!-- then the input values -->
END

for NUM in ${a[@]}
do
  echo '<record type="data" mode="textual" interface="C4SNet">'
  echo '  <field label="inval" >(int)'$NUM'</field>'
  echo '</record>'
done

echo
echo '<record type="terminate" />'
