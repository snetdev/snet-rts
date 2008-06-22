include $(SNETBASE)/src/makefiles/config.mkf

# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =

CC         = gcc
SNETC      = $(SNETBASE)/bin/snetc
CCFLAGS    = -DFILTER_VERSION_2
INCDIRS    = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS    = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS       = -lC4SNet -lpthread -lsnet 

all:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) -o $(TARGET) $(TARGET).snet
	rm -f *.o

gcc:    
	$(HIDE) rm -f *.o
	$(SNETC) -b7 $(TARGET).snet
	$(CC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) $(RPATH) -DSNetMain__factorial=main -o $(TARGET) *.c $(LIBS)
	rm -f *.o


clean:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
