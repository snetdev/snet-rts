TARGET  = libboxes.a

BOXOBJS = globalSettings.o get_MB.o dct.o huffman.o openBMP.o zzq.o saveResult.o

.PHONY: clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) -std=c99 $(CCFLAGS) -I$(SNET_INCLUDES) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
