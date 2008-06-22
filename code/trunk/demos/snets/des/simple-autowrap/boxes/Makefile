# DES Demo - Makefile for SAC boxes
#
# Stephan Herhut (s.a.herhut@herts.ac.uk)
# Frank Penczek (f.penczek@herts.ac.uk)
#
# $Id: Makefile 33 2008-02-15 11:11:24Z sah $


# some values that might need to be adopted to the current project
# 
MAINLIB := simpleDesboxes
EXCLUDE := Boxes
S2SLIB  := $(SNETBASE)/demos/interfaces/SAC

# usually there is no need to change anything below this line
#
LIBOUT := libs
INCOUT := include
SAC2C := sac2c
SAC2CFLAGS := -no cf -nophm -L $(LIBOUT) -L $(S2SLIB)

SAC4C := sac4c
SAC4CFLAGS := -L $(LIBOUT) -L $(S2SLIB) -incdir $(INCOUT) -libdir $(LIBOUT) 

LIBS := $(filter-out $(MAINLIB) $(EXCLUDE), $(patsubst %.sac,%, $(wildcard *.sac)))

.PHONY: all clean

all: $(INCOUT)/$(MAINLIB).h

clean:
	rm -f $(foreach FILE, $(LIBS), $(LIBOUT)/lib$(FILE)Tree.so)
	rm -f $(foreach FILE, $(LIBS), $(LIBOUT)/lib$(FILE)Mod.so)
	rm -f $(foreach FILE, $(LIBS), $(LIBOUT)/lib$(FILE)Mod.a)
	rm -f $(LIBOUT)/lib$(MAINLIB)Tree.so
	rm -f $(LIBOUT)/lib$(MAINLIB)Mod.so
	rm -f $(LIBOUT)/lib$(MAINLIB)Mod.a
	rm -f $(LIBOUT)/lib$(MAINLIB).so
	rm -f $(LIBOUT)/lib$(MAINLIB).a
	rm -f $(INCOUT)/$(MAINLIB).h

$(INCOUT)/$(MAINLIB).h $(LIBOUT)/lib$(MAINLIB).so \
$(LIBOUT)/lib$(MAINLIB).a: $(LIBOUT)/lib$(MAINLIB)Tree.so
	$(SAC4C) $(SAC4CFLAGS) -o $(MAINLIB) $(MAINLIB)

$(LIBOUT)/lib$(MAINLIB)Tree.so: $(foreach LIB,$(LIBS),$(LIBOUT)/lib$(LIB)Tree.so)

$(LIBOUT)/lib%Mod.a $(LIBOUT)/lib%Mod.so $(LIBOUT)/lib%Tree.so: %.sac
	$(SAC2C) $(SAC2CFLAGS) -o $(LIBOUT) $<  
