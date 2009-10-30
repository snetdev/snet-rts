
IFNAME := SAC4SNet

include $(SNETBASE)/src/makefiles/config.mkf
INCDIRS   = -I.  -I$(SNETBASE)/include \
		 -I$(SNETBASE)/src/util/metadata \
		 -I$(SAC2CBASE)/include \
		 -I$(SAC2CBASE)/src/include

MPIINCDIRS = -I$(MPIROOT)/include

LIBDIR	= sac_lib
LIB		  = $(IFNAME)FibreIO

SAC2C	  	= sac2c
SACFLAGS  = -v0

SACTARGET	= $(IFNAME)FibreIO
SAC4C			= sac4c
S4CFLAGS  = -incdir sac_include -libdir sac_lib -o $(SACTARGET)
S4CINCS		= `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS		= `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)`

DISTFLAGS = -DDISTRIBUTED_SNET

COPY      = cp

# = = = = = = = = = = = = = = = = = = = =
OBJECTS     = $(IFNAME).o
TARGET      = lib$(IFNAME).so
WRAPOBJECTS = $(IFNAME)c.o
WRAPTARGET  = lib$(IFNAME)c.so 
# = = = = = = = = = = = = = = = = = = = =
TARGETDIR   = $(SNETBASE)/lib
# = = = = = = = = = = = = = = = = = = = =


all: libSNetMod.so sac_lib/lib$(IFNAME)FibreIO.so
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) -c *.c
	$(CC) $(LD_DYNAMIC) $(S4CLIBS) -o $(TARGET) $(OBJECTS) 
	$(CC) $(LD_DYNAMIC) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(LIBDIR)/lib$(LIB).so $(TARGETDIR)
	$(COPY) $(IFNAME).h $(SNETBASE)/include

libSNetMod.so: SNet.sac
	$(SAC2C) $(SACFLAGS) SNet.sac

sac_lib/lib$(IFNAME)FibreIO.so: $(IFNAME)FibreIO.sac 
	$(SAC2C) $(SACFLAGS) $(SACTARGET).sac
	$(SAC4C) $(S4CFLAGS) $(SACTARGET)

mpi: libSNetMod.so sac_lib/lib$(IFNAME)FibreIO.so
	$(CC) $(CCFLAGS) $(DISTFLAGS) $(INCDIRS) $(MPIINCDIRS) $(S4CINCS) -c *.c
	$(CC) $(LD_DYNAMIC) $(S4CLIBS) -o $(TARGET) $(OBJECTS) 
	$(CC) $(LD_DYNAMIC) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(LIBDIR)/lib$(LIB).so $(TARGETDIR)
	$(COPY) $(IFNAME).h $(SNETBASE)/include

clean:
	rm -f *.o *.a *.so $(TARGETDIR)/$(TARGET) $(TARGETDIR)/$(WRAPTARGET) sac_include/* $(LIBDIR)/* $(SNETBASE)/include/$(IFNAME).h
