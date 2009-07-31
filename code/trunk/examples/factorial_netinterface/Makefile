include $(SNETBASE)/src/makefiles/config.mkf

# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =

CC         = gcc
MPICC      = mpicc
SNETC      = $(SNETBASE)/bin/snetc
METADATA   = -m$(TARGET).md 
DISTFLAGS  = -distributed
MPIFLAGS   = -DDISTRIBUTED_SNET
CCFLAGS    = -DFILTER_VERSION_2
INCDIRS    = -I. -I$(SNETBASE)/include
LIBDIRS    = -L. -L$(SNETBASE)/lib
LIBS       = -lC4SNet -lpthread -lsnet -lsnetutil $(SOCKETLIBS)
MPILIBS    = -lC4SNet -lpthread -lsnetmpi -lsnetutil $(SOCKETLIBS)

all:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) -o $(TARGET) $(METADATA) $(TARGET).snet
	rm -f *.o

gcc:    
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) -b7 $(METADATA) $(TARGET).snet
	$(CC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) $(RPATH) -DSNetMain__factorial=main -o $(TARGET) *.c $(LIBS)
	rm -f *.o

mpi:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) $(CCFLAGS) $(DISTFLAGS) $(INCDIRS) $(LIBDIRS) -o $(TARGET) $(METADATA) $(TARGET).snet
	rm -f *.o

mpigcc:    
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) -b7 $(METADATA) $(DISTFLAGS) $(TARGET).snet
	$(MPICC) $(CCFLAGS) $(MPIFLAGS) $(INCDIRS) $(LIBDIRS) $(RPATH) -DSNetMain__factorial=main -o $(TARGET) *.c $(MPILIBS)
	rm -f *.o

clean:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
