TARGET  = libboxes.a

BOXOBJS = init.o merger.o multiply.o split.o

.PHONY: clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) -std=c99 $(CCFLAGS) -I$(SNET_INCLUDES) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
