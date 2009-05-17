include $(SNETBASE)/src/makefiles/config.mkf
INCDIRS   = -I. \
	    -I$(SNETBASE)/include \
            -I$(SNETBASE)/src/util/metadata \
	    -I$(SAC2CBASE)/include
SAC2C	  = sac2c
SACFLAGS  = -v0
COPY      = cp

# = = = = = = = = = = = = = = = = = = = =
OBJECTS     = SAC4SNet.o
TARGET      = libSAC4SNet.so
WRAPOBJECTS = SAC4SNetc.o
WRAPTARGET  = libSAC4SNetc.so 
# = = = = = = = = = = = = = = = = = = = =
TARGETDIR   = $(SNETBASE)/lib
# = = = = = = = = = = = = = = = = = = = =


all:
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(SAC2C) $(SACFLAGS) SNet.sac
	$(CC) $(LD_DYNAMIC) -o $(TARGET) $(OBJECTS) 
	$(CC) $(LD_DYNAMIC) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(TARGETDIR)

clean:
	rm -f *.o *.a *.so $(TARGETDIR)/$(TARGET) $(TARGETDIR)/$(WRAPTARGET)
