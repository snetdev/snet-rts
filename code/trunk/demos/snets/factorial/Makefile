CC        = gcc
CCFLAGS  = -Wall -g -DFILTER_VERSION_2
SNETC     = snetc
SNETCFLAGS= -b7 
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS      = -lpthread -lC4SNet -lsnet 
RPATH     = -R$(SNETBASE)/demos/interfaces/C

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(LIBS) 

clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
