include $(SNETBASE)/src/makefiles/config.mkf

TARGET	=	minic

.PHONY: clean

all:
			gcc -g -I. -I$(SNETBASE)/include -c dec.c
			gcc -g -I. -I$(SNETBASE)/include -c init.c
			snetc -g -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib $(SOCKETLIBS) minic.snet -o minic


clean:
			- rm -f $(TARGET) *.o
