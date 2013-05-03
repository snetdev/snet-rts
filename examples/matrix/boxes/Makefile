TARGET  = libboxes.a
CFLAGS	= -std=gnu99 -Wall -O1 -g -I$(SNET_INCLUDES)
BOXOBJS = $(TARGET)(init.o) $(TARGET)(merger.o) $(TARGET)(multiply.o) $(TARGET)(split.o)
ARFLAGS = rcs

.PHONY: clean

$(TARGET): $(BOXOBJS)

clean:
	rm -f $(TARGET) $(BOXOBJS)
