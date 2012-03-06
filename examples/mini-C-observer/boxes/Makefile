TARGET  = libboxes.a

BOXOBJS = dec.o init.o

.PHONY: clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) -std=c99 $(CCFLAGS) -I$(SNET_INCLUDES) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
