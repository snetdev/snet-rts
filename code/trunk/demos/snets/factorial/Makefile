CC        = gcc
CCFLAGS  = -Wall -g -DFILTER_VERSION_2
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS      = -lpthread -lC4SNet -lsnet -lsocket -lnsl 

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS)

clean:
	rm -f *.o $(TARGET)
