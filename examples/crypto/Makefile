CC	   = gcc -std=gnu11
CFLAGS     = -g -O1 -Wall -I$(SNET_INCLUDES)
TARGETS    = crypto
BOXLIB     = libcrypto.a
ARFLAGS	   = rc
DEPS       = $(BOXLIB)
SNETCFLAGS = -g -O1 -v1 -lcrypto -lcrypt -static
CLEANS	   = $(DEPS)

include $(SNET_MISC)/snet.mkf

$(BOXLIB): $(BOXLIB)(algorithm.o) $(BOXLIB)(split.o)

