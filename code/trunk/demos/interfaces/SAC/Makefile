CC        = gcc
CCFLAGS  = -Wall -g
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SAC2CBASE)/include
SAC2C	  = sac2c
SACFLAGS  = -v0

# = = = = = = = = = = = = = = = = = = = =
TARGET    = libSAC2SNet.a
# = = = = = = = = = = = = = = = = = = = =



all:
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(SAC2C) $(SACFLAGS) SNet.sac
	$(AR) r $(TARGET) *.o 

clean:
	rm -f *.o *.a *.so
