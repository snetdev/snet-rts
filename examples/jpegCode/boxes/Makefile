TARGET  = libboxes.a
CFLAGS  = -std=gnu99 -g -O1 -Wall -I$(SNET_INCLUDES)
ARFLAGS = rcs
BOXOBJS = $(TARGET)(globalSettings.o) $(TARGET)(get_MB.o) $(TARGET)(dct.o) \
          $(TARGET)(huffman.o) $(TARGET)(openBMP.o) \
					$(TARGET)(zzq.o) $(TARGET)(saveResult.o)

.PHONY: clean

$(TARGET): $(BOXOBJS)

clean:
	rm -f $(TARGET) *.o core

