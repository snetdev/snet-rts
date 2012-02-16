TARGETS    = minic
DEPS       = boxes/libboxes.a
SNETCFLAGS = -g -Lboxes -lboxes

include $(SNET_MISC)/snet.mkf

$(DEPS):
	make -C boxes/
