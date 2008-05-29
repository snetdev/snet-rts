# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =

CC         = gcc
SNETC      = $(SNETBASE)/bin/snetc
CCFLAGS    = -DFILTER_VERSION_2  -Dmain_$(TARGET)=main
INCDIRS    = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS    = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS       = -lC2SNet -lpthread -lsnet


all:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
	$(CC) $(INCDIRS) -c *.c
	$(SNETC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) -o $(TARGET) $(TARGET).snet
	rm -f *.o

clean:
	$(HIDE) rm -f *.o $(TARGET) $(TARGET).h $(TARGET).c
