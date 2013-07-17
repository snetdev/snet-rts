TARGETS    = jpegCode
DEPS       = boxes/libboxes.a
SNETCFLAGS = -v1 -g -Lboxes -lboxes
CLEANS     = $(DEPS)

include $(SNET_MISC)/snet.mkf

$(DEPS): boxes/Makefile boxes/ColorConversion.c boxes/dct.c boxes/ejpgl.c \
	 boxes/huffman.c boxes/jpeg.c boxes/pc.c boxes/saveResult.c boxes/zzq.c
	make -C boxes

test: 
	@for f in ./jpegCode*nodist; do \
	    echo "### $$f"; \
	    time -p ./run data/test.bmp /dev/null $$f -o /dev/null >/dev/null || \
	    mv -vf core core-$${f#*jpegCode} 2>/dev/null; \
	done

