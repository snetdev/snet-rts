include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
CCFLAGS   = -Wall -g -DFILTER_VERSION_2
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I./include 
LIBDIRS   = -L. -L$(SNETBASE)/lib -L./lib -L$(SNETBASE)/src/interfaces/sac4snet
LIBS      = -lpthread -lsnet -ldistribnodist -ltbpthread -lSAC4Snet -lsnetutil

SACNAMES  = -DSACTYPE_SNet_SNet=20 -DSNetMain__sudokusolve=main

# SAC SimpleFibre library
FIBREOBJS = $(SACBASE)/stdlib/world/stdio/src/FibreIO/*.o


SNETC	  = snetc
SCFLAGS   = -b7 -v1


# - - - - - - - - - - - - - - - - - - - -
SACTARGET = sudoku
# - - - - - - - - - - - - - - - - - - - -

SAC2C     = sac2c
S2CFLAGS  = -v1 -g
SAC4C     = sac4c
S4CFLAGS  = -v1 -incdir include -libdir lib -g $(LIBDIRS) 
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)` 


# = = = = = = = = = = = = = = = = = = = =
TARGET    = sudokusolve
# = = = = = = = = = = = = = = = = = = = =

all: $(TARGET) 

$(TARGET): Objects libsudokuMod.a
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS) $(S4CLIBS) $(LIBS) $(FIBREOBJS)

libsudokuMod.a: sudoku.sac
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)

$(TARGET).c: $(TARGET).snet
	$(SNETC) $(SCFLAGS) $(TARGET).snet

Objects: main.c $(TARGET).c
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) -c *.c

clean:
	rm -f *.o *.so *.a  lib/*.so lib/*.a  include/cwrapper.h $(TARGET) $(TARGET).c $(TARGET).h 
