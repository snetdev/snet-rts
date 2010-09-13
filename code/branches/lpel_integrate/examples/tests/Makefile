include $(SNETBASE)/src/makefiles/config.mkf

TARGET	= singlebox
ALL_LIBS = $(SOCKETLIBS) $(CAPLIBS) $(PCLLIBS) $(RTLIBS)

.PHONY: clean

singlebox:  singlebox.snet
	gcc -I. -I$(SNETBASE)/include -c dec.c
	gcc -I. -I$(SNETBASE)/include -c init.c
	snetc -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib $(ALL_LIBS) $(TARGET).snet -o $(TARGET)

parallel:  parallel.snet
	gcc -I. -I$(SNETBASE)/include -c dec.c
	gcc -I. -I$(SNETBASE)/include -c inc.c
	snetc -I. -I$(SNETBASE)/include -L. -L$(SNETBASE)/lib $(ALL_LIBS) parallel.snet -o parallel

clean:
	- rm -f $(TARGET) *.o
