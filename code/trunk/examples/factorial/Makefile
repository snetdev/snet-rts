include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
MPICC     = mpicc
CCFLAGS   = -Wall -g 
SNETC     = snetc
AR        = ar
DISTFLAGS = -distributed
INCDIRS   = -I. -I$(SNETBASE)/include 
LIBDIRS   = -L. -L$(SNETBASE)/lib 
LIBS      = -lpthread -lC4SNet -lsnet -lsnetutil $(SOCKETLIBS)
FILES     = condif.c leq1.c mult.c sub.c

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
	$(SNETC) $(INCDIRS) -m $(TARGET).md -C $(TARGET).snet 
	$(CC) $(CCFLAGS) $(INCDIRS) $(LIBDIRS) $(RPATH) -o $(TARGET) *.c $(LIBS)
net: 
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
	$(CC) $(CCFLAGS) $(INCDIRS) -c $(FILES)
	$(SNETC) $(INCDIRS) $(LIBDIRS) -m $(TARGET).md -o $(TARGET) $(TARGET).snet 

netmpi: 
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
	$(CC) $(CCFLAGS) $(INCDIRS) -c $(FILES)
	$(SNETC) $(DISTFLAGS) $(INCDIRS) $(LIBDIRS) -m $(TARGET).md -o $(TARGET) $(TARGET).snet 

clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
