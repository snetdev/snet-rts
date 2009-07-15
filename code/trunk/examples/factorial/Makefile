include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
CCFLAGS  = -Wall -g 
SNETC     = snetc
SNETCFLAGS= -b7
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include 
LIBDIRS   = -L. -L$(SNETBASE)/lib 
LIBS      = -lpthread -lC4SNet -lC4SNetc -lsnet -lsnetutil $(SOCKETLIBS)

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(SNETC) $(SNETCFLAGS) -m $(TARGET).md $(TARGET).snet 
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(LIBS) 

clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
