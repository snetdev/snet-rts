# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =

SNETC      = $(SNETBASE)/bin/snetc
CCFLAGS    = -DFILTER_VERSION_2 -Dmain_$(TARGET)=main
AR         = ar
INCDIRS    = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS    = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS       = -lpthread -lsnet -lC2SNet


all:
	$(SNETC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) $(TARGET).snet
	rm -f *.o

clean:
	$(HIDE) rm -f *.o $(TARGET)
