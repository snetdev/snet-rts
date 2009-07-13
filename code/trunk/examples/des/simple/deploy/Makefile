# DES Demo - Makefile for Deployer
#
# Stephan Herhut (s.a.herhut@herts.ac.uk)
# Frank Penczek (f.penczek@herts.ac.uk)
#
# $Id: Makefile 14 2008-02-14 14:10:05Z sah $



# --- general setup
CC        = gcc
CCFLAGS   = -Wall -g -DFILTER_VERSION_2
SNETC	  = snetc
SCFLAGS   = -b7 -v0
AR        = ar

# --- S-Net Runtime / Interface Paths and Libraries
INCDIRS   = -I. -I$(SNETBASE)/include \
	    -I./include -I$(SACBASE)/sac2c/include -I../boxes/include\
	    -I../snet
LIBDIRS   = -L. -L$(SNETBASE)/lib \
	    -L../boxes/lib 
LIBS      = -lpthread -lsnet -lSAC2SNet -lsimpleDes -lm -lsocket  -lnsl


# --- Directory for S-Net 
SNETINCPATH   = ../snet
SNETLIBPATH   = ../snet

# --- Directory for Boxes
BOXINCPATH    = ../boxes/include
BOXLIBPATH    = ../boxes/libs
BOXINCS       = `sac4c -v1 -L$(BOXLIBPATH) -incdir $(BOXINCPATH) -ccflags -o simpleDesboxes simpleDesboxes`
BOXLIBS       = `sac4c -v1 -L$(BOXLIBPATH) -L$(LIBDIRS) -libdir $(BOXLIBPATH) -ldflags -o simpleDesboxes simpleDesboxes`


# ============================================================================
DEPLOYSRC    = deploy.c
TARGET       = simpleDes
# ============================================================================


.PHONY: clean 

all: deployerObjs
	$(CC) $(LIBDIRS) -L$(SNETLIBPATH) *.o $(BOXLIBS) $(LIBS) -o $(TARGET) $(LIBS)
	
deployerObjs: $(DEPLOYSRC)
	$(CC) $(CCFLAGS) $(INCDIRS) -I$(SNETINCPATH) -c $^

clean:
	rm -f *.o $(TARGET)
