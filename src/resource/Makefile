
.PHONY: all clean proto

all clean:
	$(MAKE) -C ../.. $@

proto:
	getproto -C -H resproto.h `ls *.c | sort` > $$$$ && \
	mv $$$$ resproto.h
	rm -f restypes.h
	grep -h '^typedef ' *.[ch] > restypes.h

ctags:
	ctags *.[ch]

