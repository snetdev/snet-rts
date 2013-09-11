CFLAGS     = -g -O1 -Wall -I$(SNET_INCLUDES)
TARGETS    = crypto
BOXLIB     = libcrypto.a
ARFLAGS	   = rcs
DEPS       = $(BOXLIB)
SNETCFLAGS = -g -O1 -v1 -lcrypto -lcrypt
CLEANS	   = $(DEPS)

include $(SNET_MISC)/snet.mkf

$(BOXLIB): $(BOXLIB)(algorithm.o) $(BOXLIB)(split.o) $(BOXLIB)(readfiles.o)

test: 
	@for f in ./crypto*nodist; do \
	    echo "### $$f"; \
	    time -p ./run $$f -o /dev/null >/dev/null || \
	    mv -vf core core-$${f#*crypto} 2>/dev/null; \
	done

