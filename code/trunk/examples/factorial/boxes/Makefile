
TARGET  = libboxes.a

BOXOBJS = condif.o leq1.o mult.o sub.o


CC      = gcc
CCFLAGS = -Wall -g
AR      = ar
INCDIRS = -I. -I$(SNETBASE)/include


.PHONY: all clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) $(CCFLAGS) $(INCDIRS) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
