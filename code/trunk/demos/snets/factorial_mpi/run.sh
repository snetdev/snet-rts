#!/bin/sh

if [ $# -eq 1 ]; then   
    mpirun -np $1 ./factorial    
elif [ $# -eq 2 ]; then
    if [ x"$2" = x-v ]; then
	mpirun -np $1 valgrind --leak-check=full --show-reachable=yes --num-callers=8 ./factorial     
    else
	echo 'Usage: run.sh <N> [<-v> [<suppressions file>]]';
    fi
elif [ $# -eq 3 ]; then
    if [ x"$2" = x-v ]; then
	mpirun -np $1 valgrind --leak-check=full --show-reachable=yes --num-callers=8 --suppressions=$3 ./factorial     
    else
	echo 'Usage: run.sh <N> [<-v> [<suppressions file>]]';
    fi
else
    echo 'Usage: run.sh <N> [<val> [<suppressions file>]]';
fi