TARGETS    = poweroftwo poweroftwo2
DEPS       = 
SNETCFLAGS = -v1 -g

include $(SNET_MISC)/snet.mkf

test: 
	@for f in ./poweroftwo*nodist; do \
	    echo "### $$f"; \
	    time -p ./run 15 $$f -o /dev/null || \
	    mv -vf core core-$${f#*poweroftwo} 2>/dev/null; \
	done
