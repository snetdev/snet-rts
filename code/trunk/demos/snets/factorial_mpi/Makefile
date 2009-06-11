CC        = mpicc
CCFLAGS  = -Wall -g -DFILTER_VERSION_2 -DDISTRIBUTED_SNET
SNETC     = snetc
SNETCFLAGS= -b7 -distributed
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C -L$(MPIROOT)/lib
LIBS      = -lpthread -lC4SNet -lC4SNetc -lmpi -lsnetmpi -lsnetutil

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all:   
	$(SNETC) $(SNETCFLAGS) -m $(TARGET).md $(TARGET).snet 
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(LIBS)
clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
