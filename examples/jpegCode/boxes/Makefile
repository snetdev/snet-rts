TARGET  = libboxes.a

BOXOBJS = ejpgl.o ColorConversion.o dct.o huffman.o jpeg.o pc.o zzq.o saveResult.o

.PHONY: clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) -std=c99 $(CCFLAGS) -I$(SNET_INCLUDES) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
