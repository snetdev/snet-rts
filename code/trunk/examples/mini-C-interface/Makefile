TARGET	=	minic

.PHONY: clean

all:
			gcc -I. -I$(SNETBASE)/include -c dec.c
			gcc -I. -I$(SNETBASE)/include -c init.c
			snetc -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib minic.snet -o minic


clean:
			- rm -f $(TARGET) *.o
