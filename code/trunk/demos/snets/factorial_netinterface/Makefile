CC         = gcc
CCFLAGS    = -Wall -g -DFILTER_VERSION_2
AR         = ar
INCDIRS    = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS    = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C -L./lib
LIBS       = -lpthread -lsnet -lC2SNet

# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS)
	rm -f *.o

clean:
	$(HIDE) rm -f *.o $(TARGET)
