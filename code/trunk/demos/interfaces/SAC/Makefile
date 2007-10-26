CC        = gcc
CCFLAGS  = -Wall -g
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SAC2CBASE)/include



# = = = = = = = = = = = = = = = = = = = =
TARGET    = libSAC2SNet.a
# = = = = = = = = = = = = = = = = = = = =



all:
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(AR) r $(TARGET) *.o 

clean:
	rm -f *.o *.a
