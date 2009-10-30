include $(SNETBASE)/src/makefiles/config.mkf

CC        = mpicc
CCFLAGS   = -Wall -g -DDISTRIBUTED_SNET -DFILTER_VERSION_2 $(RPATH)
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I./include -I$(MPIROOT)/include
LIBDIRS   = -L. -L$(SNETBASE)/lib -L./lib -L$(SNETBASE)/interfaces/SAC
LIBS      = -lpthread -lsnetmpi -lSAC4SNet -lsnetutil

SACNAMES  = -DSACTYPE_SNet_SNet=20 -DSNetMain__mini=main

SNETC      = snetc
SNETCFLAGS = -b7 -distributed

# - - - - - - - - - - - - - - - - - - - -
SACTARGET = mini
# - - - - - - - - - - - - - - - - - - - -

SAC2C     = sac2c
S2CFLAGS  = -v1 -g
SAC4C     = sac4c
S4CFLAGS  = -v3 -incdir include -libdir lib -g $(LIBDIRS) -d nocleanup -d cccall
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)` 


# = = = = = = = = = = = = = = = = = = = =
TARGET    = mini
# = = = = = = = = = = = = = = = = = = = =



all: lib$(SACTARGET)Mod.so $(TARGET).c
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(SACNAMES) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(LIBS) $(S4CLIBS) $(LIBS)

lib$(SACTARGET)Mod.so: $(SACTARGET).sac
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)

$(TARGET).c: $(TARGET).snet
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet

clean:
	rm -f *.o *.so *.a  lib/*.so lib/*.a  include/cwrapper.h $(TARGET) cwrapper.sac2c $(TARGET).?
