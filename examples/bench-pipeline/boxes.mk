BOXES = \
	boxes/conv.c \
	boxes/add.c \
	boxes/gen.c \
	boxes/drop.c

# if you change the following, also change run.mk to update
# snetc's linker flags.
BOXLIB = libboxes.a

.PRECIOUS: $(BOXLIB)

$(BOXLIB): $(BOXES:.c=.o)
	$(AR) cru $@ $^
	ranlib $@

.PHONY: clean-boxes
clean-boxes:
	rm -f boxes/*.o $(BOXLIB)
