
.PHONY: all proto

all:
	@rm -f -- core*
	$(MAKE) -C ../.. $@

proto:
	getproto -C -H resproto.h `ls *.c | sort` > $$$$ && \
	mv $$$$ resproto.h
	rm -f restypes.h
	grep -h '^typedef ' *.[ch] > restypes.h

clean:
	rm -rf -- core* *.o .deps .libs .dirstamp

ctags:
	ctags *.[ch]

