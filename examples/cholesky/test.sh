#!/bin/bash

pi=2
for i in {1..12}
do
  pj=2
  for j in `seq 1 $i`
  do
    output=`python data/generator.py ${pi} ${pj} foo; ./cholesky-lpel-nodist -i foo_snet_in.xml | grep elapsed | grep -Eo '[0-9]+\.[-+e0-9]+'`
    echo $(printf "%d" "$pi") $(printf "%d" "$pj") $(printf "%.10f" "$output")
    pj=$(( pj * 2 ))
  done
  pi=$(( pi * 2 ))
done
rm foo_snet_in.xml foo_snet_out.xml
