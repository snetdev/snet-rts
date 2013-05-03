TARGET  = libboxes.a
CFLAGS  = -std=gnu99 -g -O1 -Wall -I$(SNET_INCLUDES)
ARFLAGS = rcs
BOXOBJS = $(TARGET)(ejpgl.o) $(TARGET)(ColorConversion.o) $(TARGET)(dct.o) \
          $(TARGET)(huffman.o) $(TARGET)(jpeg.o) $(TARGET)(pc.o) \
	  $(TARGET)(zzq.o) $(TARGET)(saveResult.o)

.PHONY: clean

$(TARGET): $(BOXOBJS)

clean:
	rm -f $(TARGET) *.o core

