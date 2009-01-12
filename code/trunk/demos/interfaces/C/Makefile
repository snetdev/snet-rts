include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
MPICC     = mpicc
CCFLAGS   = -Wall -g -fPIC
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include
SOFLAGS   = -shared -dy -Wl -allow-shlib-undefined 
COPY      = cp
DISTFLAGS = -DDISTRIBUTED_SNET

# = = = = = = = = = = = = = = = = = = = =
OBJECTS     = C4SNet.o base64.o 
TARGET      = libC4SNet.so 
WRAPOBJECTS = C4SNetc.o
WRAPTARGET  = libC4SNetc.so
# = = = = = = = = = = = = = = = = = = = =
TARGETDIR   = $(SNETBASE)/lib
# = = = = = = = = = = = = = = = = = = = =


all:
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(SOFLAGS) -o $(TARGET) $(OBJECTS) 
	$(CC) $(SOFLAGS) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(TARGETDIR)

mpi:
	$(MPICC) $(MPIFLAGS) $(DISTFLAGS) $(CCFLAGS) $(INCDIRS) -c *.c
	$(MPICC) $(SOFLAGS) -o $(TARGET) $(OBJECTS) 
	$(MPICC) $(SOFLAGS) -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(TARGETDIR)

clean:
	rm -f *.o *.a *.so $(TARGETDIR)/$(TARGET) $(TARGETDIR)/$(WRAPTARGET)
