include $(SNETBASE)/src/makefiles/config.mkf

TARGET	=	minic

.PHONY: clean

all:
	gcc -I. -I$(SNETBASE)/include -c dec.c
	gcc -I. -I$(SNETBASE)/include -c init.c
	snetc -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib $(SOCKETLIBS) $(CAPLIBS) $(PCLLIBS) $(RTLIBS) minic.snet -o minic


clean:
	- rm -f $(TARGET) *.o
