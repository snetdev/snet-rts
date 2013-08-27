
.PHONY: all clean proto

all clean:
	$(MAKE) -C ../.. $@

proto:
	getproto -C -H resproto.h `ls *.c | sort` > $$$$ && mv $$$$ resproto.h

