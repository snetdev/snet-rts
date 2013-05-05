TARGETS    = matrix
BOXES      = boxes/libboxes.a
DEPS	   = $(BOXES) inputrecord
SNETCFLAGS = -v1 -g -Lboxes -lboxes
CFLAGS	   = -std=gnu99 -Wall -O1 -g
CLEANS	   = $(DEPS) boxes/*.o

include $(SNET_MISC)/snet.mkf

$(BOXES): boxes/Makefile boxes/init.c boxes/merger.c boxes/multiply.c boxes/split.c
	make -C boxes

inputrecord: inputrecord.c

test: inputrecord
	@./inputrecord -w 1024 -h 1024 -n 12 -o input.xml
	@for f in ./matrix*nodist; do \
	    echo "### $$f"; \
	    time -p $$f -i input.xml -o /dev/null || \
	    mv -vf core core-$${f#*matrix} 2>/dev/null; \
	done

