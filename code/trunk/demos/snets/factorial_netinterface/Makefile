CC        = gcc
BISON     = bison
BFLAGS	  = -d
FLEX      = flex
FFLAGS	  =
CCFLAGS   = -Wall -g -DFILTER_VERSION_2
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C
LIBDIRS   = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C
LIBS      = -lpthread -lsnet -lC2SNet

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(BISON) $(BFLAGS) *.y
	$(FLEX) $(FFLAGS) *.l
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS)
	rm -f *.tab.* lex.yy.c *.o

clean:
	rm -f *.o $(TARGET)
