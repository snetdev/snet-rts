include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
CCFLAGS  = -Wall -g 
SNETC     = snetc
SNETCFLAGS= -b7
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS      = -lpthread -lC4SNet -lC4SNetc -lsnet -lsnetutil

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(SNETC) $(SNETCFLAGS) -m $(TARGET).md $(TARGET).snet 
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(LIBS) 

clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
