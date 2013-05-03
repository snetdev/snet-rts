TARGET  = libboxes.a
CFLAGS  = -std=gnu99 -Wall -g -I$(SNET_INCLUDES)
BOXOBJS = $(TARGET)(Split.o) $(TARGET)(Multiplication.o) $(TARGET)(Merger.o)
ARFLAGS = rcs

.PHONY: clean

$(TARGET): $(BOXOBJS)

clean:
	rm -f $(TARGET) $(BOXOBJS)
