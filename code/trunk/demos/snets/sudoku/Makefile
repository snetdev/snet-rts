CC        = gcc
CCFLAGS   = -Wall -g -DFILTER_VERSION_2
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/SAC\
	    -I./include 
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/SAC\
	    -L./lib
LIBS      = -lpthread -lsnet -lSAC2SNet

# SAC SimpleFibre library
FIBREOBJS = $(SACBASE)/stdlib/world/stdio/src/FibreIO/*.o

# - - - - - - - - - - - - - - - - - - - -
SACTARGET = sudoku
# - - - - - - - - - - - - - - - - - - - -

SAC2C     = sac2c
S2CFLAGS  = -v1 -g
SAC4C     = sac4c
S4CFLAGS  = -v3 -incdir include -libdir lib -g $(LIBDIRS) -d nocleanup -d cccall
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)` 


# = = = = = = = = = = = = = = = = = = = =
TARGET    = sudokusolve
# = = = = = = = = = = = = = = = = = = = =



all: 
#	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $(SACTARGET).sac
#	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) -c *.c
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS) $(S4CLIBS) $(LIBS) $(FIBREOBJS)

clean:
	rm -f *.o *.so *.a  lib/*.so lib/*.a  include/cwrapper.h $(TARGET)
