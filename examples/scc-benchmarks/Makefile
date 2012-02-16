CFLAGS     = -O0 -g -std=c99
TARGETS    = parallel
DEPS       = libboxes.a
SNETCFLAGS = -O0 -g -lboxes -static

include $(SNET_MISC)/snet.mkf

libboxes.a: boxes.o
	ar rc $@ $+

%.o: %.c
	gcc $(CFLAGS) -I$(SNET_INCLUDES) -c $<
