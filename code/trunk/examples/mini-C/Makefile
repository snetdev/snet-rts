include $(SNETBASE)/src/makefiles/config.mkf

TARGET	=	minic

.PHONY: clean

all:
			gcc -I. -I$(SNETBASE)/include -c dec.c
			gcc -I. -I$(SNETBASE)/include -c init.c
			snetc -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib $(SOCKETLIBS) minic.snet -o minic -lsnet -ltblpel -ldistribnodist


clean:
			- rm -f $(TARGET) *.o
