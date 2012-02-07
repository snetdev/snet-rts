
INCDIRS   = -I. -I$(SNET_INCLUDES) -I./include
LIBDIRS   = -L. -L$(SNET_LIBS)     -L./lib
#THREADING = pthread
THREADING = lpel
LIBS      = -lSAC4SNet -lpthread -lsnetutil -ltb$(THREADING) -ldistribnodist -lruntimestream

SACNAMES  = -DSACTYPE_SNet_SNet=20 -DSNetMain__mini=main

SNETC      = snetc
SNETCFLAGS = -b7 -threading $(THREADING)

# - - - - - - - - - - - - - - - - - - - -
SACTARGET = mini
# - - - - - - - - - - - - - - - - - - - -

CC        = gcc
SAC2C     = sac2c
S2CFLAGS  = -v1 -g
SAC4C     = sac4c
S4CFLAGS  = -v3 -incdir include -libdir lib -g $(LIBDIRS)
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)`


# = = = = = = = = = = = = = = = = = = = =
TARGET    = mini
# = = = = = = = = = = = = = = = = = = = =



all: $(TARGET)

$(TARGET): lib$(SACTARGET)Mod.so $(TARGET).c
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(SACNAMES) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(S4CLIBS) $(LIBS)

lib$(SACTARGET)Mod.so: $(SACTARGET).sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)

$(TARGET).c: $(TARGET).snet
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet

clean:
	rm -f *.o *.so *.a $(TARGET) cwrapper.sac2c $(TARGET).?
	rm -fr lib include
