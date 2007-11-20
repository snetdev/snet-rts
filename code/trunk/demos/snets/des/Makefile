CC        = gcc
CCFLAGS   = -Wall -g -DFILTER_VERSION_2
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/SAC\
	    -I./include 
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/SAC\
	    -L./lib
LIBS      = -lpthread -lsnet -lSAC2SNet


# - - - - - - - - - - - - - - - - - - - -
SACTARGET = desboxes
# - - - - - - - - - - - - - - - - - - - -
SAC2C     = sac2c
S2CFLAGS  = -v1 -nosaa  -trace mr
SAC4C     = sac4c
S4CFLAGS  = -v1 -incdir include -libdir lib
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)` -lsac_seq


# = = = = = = = = = = = = = = = = = = = =
TARGET    = des
# = = = = = = = = = = = = = = = = = = = =



all: 
	$(SAC2C) $(S2CFLAGS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(SACTARGET)
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) -c *.c
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS) $(S4CLIBS) $(FIBREOBJS)

clean:
	rm -f *.o *.so *.a  lib/*.so lib/*.a  include/cwrapper.h $(TARGET)
