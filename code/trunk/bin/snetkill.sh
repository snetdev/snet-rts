#!/bin/bash

killthem=`ps -ef | grep SNET | grep -v grep | awk '{print $2}'`

for pid in $killthem
do
	kill  $pid
done

