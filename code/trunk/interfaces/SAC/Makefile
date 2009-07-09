include $(SNETBASE)/src/makefiles/config.mkf
INCDIRS   = -I.  -I$(SNETBASE)/include \
		 -I$(SNETBASE)/src/util/metadata \
		 -I$(SAC2CBASE)/include \
		 -I$(SAC2CBASE)/src/include

LIBDIR	= sac_lib
LIB		  = SAC4SNetFibreIO

SAC2C	  	= sac2c
SACFLAGS  = -v0

SACTARGET	= SAC4SNetFibreIO
SAC4C			= sac4c
S4CFLAGS  = -incdir sac_include -libdir sac_lib -o $(SACTARGET)
S4CINCS		= `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS		= `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)`


COPY      = cp

# = = = = = = = = = = = = = = = = = = = =
OBJECTS     = SAC4SNet.o
TARGET      = libSAC4SNet.so
WRAPOBJECTS = SAC4SNetc.o
WRAPTARGET  = libSAC4SNetc.so 
# = = = = = = = = = = = = = = = = = = = =
TARGETDIR   = $(SNETBASE)/lib
# = = = = = = = = = = = = = = = = = = = =


all: libSNetMod.so sac_lib/libSAC4SNetFibreIO.so
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) -c *.c
	$(CC) $(LD_DYNAMIC) $(S4CLIBS) -o $(TARGET) $(OBJECTS) 
	$(CC) $(LD_DYNAMIC) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(LIBDIR)/lib$(LIB).so $(TARGETDIR)

libSNetMod.so: SNet.sac
	$(SAC2C) $(SACFLAGS) SNet.sac

sac_lib/libSAC4SNetFibreIO.so: SAC4SNetFibreIO.sac 
	$(SAC2C) $(SACFLAGS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(SACTARGET)


clean:
	rm -f *.o *.a *.so $(TARGETDIR)/$(TARGET) $(TARGETDIR)/$(WRAPTARGET) sac_include/* sac_lib/*
