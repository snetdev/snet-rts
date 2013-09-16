
all:
	make -C ../../..

ctags:
	mkctags

proto:
	getproto -C `ls node.c x*.c | sort` > t && diff -C 1 node-proto.h t

