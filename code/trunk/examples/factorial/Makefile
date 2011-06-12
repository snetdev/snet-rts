include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
CCFLAGS   = -Wall -g
SNETC     = snetc
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include
LIBS      = $(SOCKETLIBS) $(RPATH)
FILES     = condif.c leq1.c mult.c sub.c

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
# = = = = = = = = = = = = = = = = = = = =


all: 
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
	$(CC) $(CCFLAGS) $(INCDIRS) -c $(FILES)
	$(SNETC) -g $(LIBS) -m $(TARGET).md -o $(TARGET) $(TARGET).snet

clean:
	rm -f *.o $(TARGET) $(TARGET).c $(TARGET).h
