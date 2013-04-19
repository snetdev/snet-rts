TARGETS    = raytrace loadbalance
DEPS       = libboxes.a
SNETCFLAGS = -v 0 -g -lboxes

include $(SNET_MISC)/snet.mkf

libboxes.a: boxes.o
	ar rc $@ $+

%.o: %.c
	$(CC) $(CFLAGS) -I$(SNET_INCLUDES) -c $<
