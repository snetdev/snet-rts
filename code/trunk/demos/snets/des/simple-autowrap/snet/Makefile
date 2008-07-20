# DES Demo - Makefile for S-Net
#
# Frank Penczek (f.penczek@herts.ac.uk)
# Stephan Herhut (s.a.herhut@herts.ac.uk)
#
# $Id: Makefile 14 2008-02-14 14:10:05Z sah $



# --- general setup
CC        = gcc
CCFLAGS   = -Wall -g -DFILTER_VERSION_2 -DSACTYPE_SNet_SNet=21 
SNETC	  = snetc
SCFLAGS   = -b7 -v0
AR        = ar

# --- S-Net Runtime / Interface Paths and Libraries
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/SAC\
	    -I./include -I$(SACBASE)/sac2c/include
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/SAC\
	    -L./lib
LIBS      = -lpthread -lsnet -lSAC4SNet


# --- Include Directory for SAC Boxes
BOXINCS   = -I../boxes/include


# = = = = = = = = = = = = = = = = = = = =
TARGET    = simpleDes
# = = = = = = = = = = = = = = = = = = = =

.PHONY: all 

all: lib$(TARGET).a

lib%.a: %.o $(foreach FILE,$(WRAPFILES),$(FILE).o)
	$(AR) rc lib$*.a $^

%.c %.h: %.snet
	$(SNETC) $(SCFLAGS) $<

%.o: %.c
	$(CC) $(CCFLAGS) $(INCDIRS) $(BOXINCS) -c $<

clean:
	rm -f $(TARGET).o lib$(TARGET).a $(TARGET).c $(TARGET).h 
	rm -f $(foreach FILE,$(WRAPFILES),$(FILE).o)
