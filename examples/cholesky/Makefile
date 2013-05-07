TARGETS    = cholesky
DEPS       = boxes/libboxes.a
#SNETCFLAGS = -g -Lboxes -lboxes
SNETCFLAGS = -v1 -O3 -Lboxes -lboxes -lm

include $(SNET_MISC)/snet.mkf

$(DEPS):
	make -C boxes/

test: 
	@python data/generator.py 1024 32 foo
	@for f in ./cholesky*nodist; do \
	    echo "### $$f"; \
	    time -p ./$$f -i foo_snet_in.xml -o /dev/null || \
	    mv -vf core core-$${f#*sieve} 2>/dev/null; \
	done
