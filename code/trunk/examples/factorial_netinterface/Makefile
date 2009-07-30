include $(SNETBASE)/src/makefiles/config.mkf

# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =

CC         = gcc
SNETC      = $(SNETBASE)/bin/snetc
METADATA   = -m$(TARGET).md 
CCFLAGS    = -DFILTER_VERSION_2
VALGRIND   = valgrind
VFLAGS     = --leak-check=full --show-reachable=yes
INCDIRS    = -I. -I$(SNETBASE)/include
LIBDIRS    = -L. -L$(SNETBASE)/lib
LIBS       = -lC4SNet -lpthread -lsnet -lsnetutil $(SOCKETLIBS)

all:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) $(LIBS) -o $(TARGET) $(METADATA) $(TARGET).snet
	rm -f *.o

gcc:    
	$(HIDE) rm -f *.o
	$(SNETC) -b7 $(METADATA) $(TARGET).snet
	$(CC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) $(RPATH) -DSNetMain__factorial=main -o $(TARGET) *.c $(LIBS)
	rm -f *.o

valgrind:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(VALGRIND) $(VFLAGS) $(SNETC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) -o $(TARGET) $(METADATA) $(TARGET).snet
	rm -f *.o

clean:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
