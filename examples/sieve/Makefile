TARGETS    = sieve sieve2
DEPS       = 
SNETCFLAGS = -v1 -g

include $(SNET_MISC)/snet.mkf

test: 
	@for f in ./sieve*nodist; do \
	    echo "### $$f"; \
	    time -p ./run 1000 1000 $$f -o /dev/null || \
	    mv -vf core core-$${f#*sieve} 2>/dev/null; \
	done
