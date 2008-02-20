CC         = gcc
CCFLAGS  = -Wall -g -DFILTER_VERSION_2
AR         = ar
INCDIRS    = -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C -I./include
LIBDIRS    = -L. -L$(SNETBASE)/lib -L$(SNETBASE)/demos/interfaces/C -L./lib
LIBS       = -lpthread -lsnet -lC2SNet -lsnetninter

# = = = = = = = = = = = = = = = = = = = =
TARGET     = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	$(MAKE) -Cnetinterface/
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(LIBDIRS) -o $(TARGET) *.o $(LIBS)
	rm -f *.tab.* lex.yy.c *.o

clean:
	$(HIDE) rm -f *.o $(TARGET)
