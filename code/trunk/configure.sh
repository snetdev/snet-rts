#!/bin/bash

# TODO: solaris / linux libs
set -e
if ! pkg-config --version > /dev/null 2>/dev/null; then 
	echo "" 
	echo " >>> ERROR: pkg-config is not installed or not in your PATH. Correct that please." 
	echo "";
	exit 1; 
fi 


if ! pkg-config --cflags gtk+-2.0 > /dev/null 2>/dev/null; then 
	echo "" 
	echo " >>> ERROR: pkg-config could not find gtk+-2.0. Correct that please." 
	echo "";
	exit 1; 
fi 

if ! pkg-config --libs gtk+-2.0 > /dev/null 2>/dev/null; then 
	echo "" 
	echo " >>> ERROR: pkg-config could not find gtk+-2.0. Correct that please." 
	echo "";
	exit 1; 
fi 

INCLUDES=`pkg-config --cflags gtk+-2.0`
LIBS=`pkg-config --libs gtk+-2.0`
# SOCKETLIBS=-lsocket -lnsl -lresolv
if  uname | grep "Sun" > /dev/null 2>/dev/null; then
	echo ""
	echo " >>> Detected SunOS"
	SOCKETLIBS='-lsocket -lnsl -lresolv'
	echo "";
elif uname | grep "Linux" > /dev/null 2>/dev/null; then
	echo ""
	echo " >>> Detected Linux"
	SOCKETLIBS=''
	echo "";
else
	echo ""
	echo " >>> Could not detect OS. (Maybe you have to add libraries by hand)"
	echo "";
fi



echo GTKINCLUDE=$INCLUDES > Makefile
echo GTKLIBS=$LIBS -lgthread-2.0 >> Makefile
echo SOCKETLIBS=$SOCKETLIBS >> Makefile
cat Makefile.in >> Makefile

echo ""
echo " >>> Makefile written."
echo ""
