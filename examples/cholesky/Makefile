TARGETS    = cholesky
DEPS       = boxes/libboxes.a
#SNETCFLAGS = -g -Lboxes -lboxes
SNETCFLAGS = -v1 -O3 -Lboxes -lboxes

include $(SNET_MISC)/snet.mkf

$(DEPS):
	make -C boxes/
