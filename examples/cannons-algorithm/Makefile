TARGETS    = cannon
BOXES      = boxes/libboxes.a
DEPS       = $(BOXES) inputrecord
SNETCFLAGS = -v1 -g -Lboxes -lboxes -lm
CLEANS     = $(DEPS) boxes/*.o

include $(SNET_MISC)/snet.mkf

$(BOXES): boxes/Makefile boxes/Merger.c boxes/Multiplication.c boxes/Split.c
	make -C boxes

inputrecord: inputrecord.c

test: inputrecord
	@./inputrecord -s 500 -d 4 -o input.xml
	@for f in ./cannon*nodist; do \
	    echo "### $$f"; \
	    time -p $$f -i input.xml -o /dev/null >/dev/null || \
	    mv -vf core core-$${f#*cannon} 2>/dev/null; \
	done
