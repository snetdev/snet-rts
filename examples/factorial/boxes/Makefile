TARGET  = libboxes.a

BOXOBJS = condif.o leq1.o mult.o sub.o

CC      = gcc
CCFLAGS = -Wall -g
AR      = ar

.PHONY: all clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) $(CCFLAGS) -I$(SNET_INCLUDES) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
