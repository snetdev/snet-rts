CFLAGS     = -g
TARGETS    = crypto
DEPS       = libcrypto.a
SNETCFLAGS = -g -lcrypto -lcrypt -static

include $(SNET_MISC)/snet.mkf

libcrypto.a: algorithm.o split.o
	ar rc $@ $+

%.o: %.c
	gcc $(CFLAGS) -I$(SNET_INCLUDES) -c $<
