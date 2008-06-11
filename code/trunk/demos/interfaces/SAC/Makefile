CC        = gcc
CCFLAGS   = -Wall -g -fPIC
SOFLAGS   = -shared -dy -Wl -allow-shlib-undefined 
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SAC2CBASE)/include
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
	$(CC) $(SOFLAGS) -o $(TARGET) $(OBJECTS) 
	$(CC) $(SOFLAGS) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(TARGETDIR)

clean:
	rm -f *.o *.a *.so $(TARGETDIR)/$(TARGET) $(TARGETDIR)/$(WRAPTARGET)
