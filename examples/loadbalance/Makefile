TARGETS    = raytrace loadbalance
DEPS       = libboxes.a
CFLAGS	   = -std=gnu99 -g -O1 -Wall -I$(SNET_INCLUDES)
SNETCFLAGS = -v1 -g -lboxes -lm
CLEANS     = $(DEPS)
ARFLAGS	   = rcs

include $(SNET_MISC)/snet.mkf

$(DEPS): $(DEPS)(boxes.o)

test: 
	@for f in ./raytrace*nodist; do \
	    echo "### $$f"; \
	    time -p ./run 250 250 1 3 $$f -o /dev/null || \
	    mv -vf core core-$${f#*raytrace} 2>/dev/null; \
	done 2>&1 | grep -v 'Warning: Destroying'

