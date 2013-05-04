TARGETS    = fibonacci2 fibonacci fibonacci3 fibonacci5 fibonacci7
DEPS       = 
SNETCFLAGS = -v1 -g

include $(SNET_MISC)/snet.mkf

test: 
	@for f in ./fibonacci*nodist; do \
	    echo "### $$f"; \
	    time -p ./run 15 $$f -o /dev/null || \
	    mv -vf core core-$${f#*fibonacci} 2>/dev/null; \
	done
