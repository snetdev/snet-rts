TARGET	=	minic

.PHONY: clean

all:
			gcc -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C -c dec.c
			gcc -I. -I$(SNETBASE)/include -I$(SNETBASE)/demos/interfaces/C -c init.c
			snetc -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib -I$(SNETBASE)/demos/interfaces/C -L$(SNETBASE)/demos/interfaces/C minic.snet -o minic


clean:
			- rm -f $(TARGET) *.o
