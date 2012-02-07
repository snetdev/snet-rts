TARGETS    = factorial
DEPS       = boxes/libboxes.a
SNETCFLAGS = -g -Lboxes -lboxes

include ../makefiles/snet.mkf

$(DEPS):
	make -C boxes/
