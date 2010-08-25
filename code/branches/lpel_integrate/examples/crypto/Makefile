include $(SNETBASE)/src/makefiles/config.mkf
CC        = mpicc
CCFLAGS  = -Wall -g -DDISTRIBUTED_SNET 
SNETC     = snetc
SNETCFLAGS= -b7 -distributed
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/interfaces/C
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(MPIROOT)/lib
LIBS      = -lpthread -lC4SNet -lC4SNetc -lsnetmpi -lsnetutil -lcrypt 

# = = = = = = = = = = = = = = = = = = = =
TARGET    = crypto
# = = = = = = = = = = = = = = = = = = = =


all:   
	$(SNETC) $(SNETCFLAGS) -m $(TARGET).md $(TARGET).snet 
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) *.o $(LIBS)
clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
