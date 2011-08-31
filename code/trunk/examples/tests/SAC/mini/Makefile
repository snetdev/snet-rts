
INCDIRS   = -I. -I$(SNET_INCLUDES) -I./include \
            -I$(SNETBASE)/src/interfaces/sac4snet

LIBDIRS   = -L. -L$(SNET_LIBS)     -L./lib \
            -L$(SNETBASE)/src/interfaces/sac4snet

#THREADING = pthread
THREADING      = lpel
LIBS_LA        = libsnetutil.la libtb$(THREADING).la libdistribnodist.la libruntimestream.la
LIBSAC4SNET_LA = $(SNETBASE)/src/interfaces/sac4snet/libSAC4SNet.la

SACNAMES  = -DSACTYPE_SNet_SNet=20 -DSNetMain__mini=main

SNETC      = snetc
SNETCFLAGS = -b7 -threading $(THREADING)

#export SNET_EXTRA_LDFLAGS += -L$(SNETBASE)/src/interfaces/sac4snet

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
	libtool --mode=compile $(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(SACNAMES) -c *.c
	libtool --mode=link    $(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(S4CLIBS) $(LIBSAC4SNET_LA) $(LIBS_LA:%=$(SNETBASE)/%)

lib$(SACTARGET)Mod.so: $(SACTARGET).sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)

$(TARGET).c: $(TARGET).snet
	LD_LIBRARY_PATH=$(SNETBASE)/src/interfaces/sac4snet:$(LD_LIBRARY_PATH) && \
          $(SNETC) $(SNETCFLAGS) $(TARGET).snet

clean:
	libtool --mode=clean rm -f $(TARGET) $(TARGET).lo
	rm -f *.o *.so *.a cwrapper.sac2c $(TARGET).?
	rm -fr lib include
