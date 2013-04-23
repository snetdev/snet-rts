TARGETS    = raytrace loadbalance
DEPS       = libboxes.a
SNETCFLAGS = -v1 -g -lboxes

include $(SNET_MISC)/snet.mkf

libboxes.a: boxes.o
	ar rc $@ $+

%.o: %.c
	$(CC) $(CFLAGS) -I$(SNET_INCLUDES) -c $<
