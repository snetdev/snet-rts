TARGET  = libboxes.a
CFLAGS  = -std=gnu99 -O3 -I$(SNET_INCLUDES)
BOXOBJS = cholesky_prim.o decompose.o compute_s1.o compute_s2.o merger.o compute_s3.o update_sync.o

.PHONY: clean

$(TARGET): $(BOXOBJS)
	$(AR) rcs $@ $^

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(BOXOBJS)
