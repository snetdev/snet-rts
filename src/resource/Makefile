
.PHONY: all proto

all install:
	@rm -f -- core* vgcore*
	$(MAKE) -C ../.. $@

proto:
	@getproto -C -H resproto.h `ls *.c | sort` > $$$$ && \
	if [ ! -f resproto.h ] || ! cmp -s $$$$ resproto.h ; then \
	mv -v -- $$$$ resproto.h ; else rm -f -- $$$$ ; fi
	@grep -h '^typedef ' `ls *.c` | sort -u > $$$$ && \
	if [ ! -f restypes.h ] || ! cmp -s $$$$ restypes.h ; then \
	mv -v -- $$$$ restypes.h ; else rm -f -- $$$$ ; fi

clean:
	rm -rf -- core* vgcore* *.[oa] resserv

ctags:
	ctags *.[ch]

