CC        = gcc
CCFLAGS   = -Wall -g -DFILTER_VERSION_2
BISON     = bison
BFLAGS    = -d
FLEX      = flex
FFLAGS    =
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C -I../include/
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS      = -lpthread -lsnet -lC2SNet

# = = = = = = = = = = = = = = = = = = = =
TARGET    = libsnetninter.a
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(BISON) $(BFLAGS) *.y
	$(FLEX) $(FFLAGS) *.l
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(AR) rcs $(TARGET) *.o
	mv $(TARGET) ../lib/$(TARGET)
	rm -f *.tab.* lex.yy.c *.o

clean:
	rm -f *.o $(TARGET)
