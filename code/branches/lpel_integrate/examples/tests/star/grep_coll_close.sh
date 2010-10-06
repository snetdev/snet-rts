#!/bin/bash

cat out00*.log | sort | grep "tid 2 " | grep ",C," | less
