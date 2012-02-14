INCDIRS   = -I. -I$(SNET_INCLUDES) -I./include
LIBDIRS   = -L. -L$(SNET_LIBS) -L./lib

SACNAMES  = -DSACTYPE_SNet_SNet=20 -DSNetMain__mini=main

SNETC      = snetc
SNETCFLAGS = -b7 -threading lpel -distrib nodist

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

LT = $(SNET_LIBS)/libtool

# = = = = = = = = = = = = = = = = = = = =
TARGET    = mini
# = = = = = = = = = = = = = = = = = = = =


all: $(TARGET)

$(TARGET): lib$(SACTARGET)Mod.so $(TARGET).c
	$(LT) --mode=compile $(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(SACNAMES) -c *.c
	$(LT) --mode=link    $(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(S4CLIBS)

lib$(SACTARGET)Mod.so: $(SACTARGET).sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)

$(TARGET).c: $(TARGET).snet
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet

clean:
	$(LT) --mode=clean rm -f $(TARGET) $(TARGET).lo
	rm -f *.o *.so *.a cwrapper.sac2c $(TARGET).?
	rm -fr lib include
