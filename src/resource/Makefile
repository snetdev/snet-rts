
.PHONY: all clean proto

all clean:
	$(MAKE) -C ../.. $@

proto:
	getproto -C -H resproto.h `ls *.c | sort` > $$$$ && \
	mv $$$$ resproto.h && \
	grep -h '^typedef ' *.c > $$$$ && \
	mv $$$$ restypes.h

ctags:
	ctags *.[ch]

