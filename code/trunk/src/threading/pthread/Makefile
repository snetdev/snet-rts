#
# create a shared library
#

CFLAGS = -c -g -Wall -pthread -fPIC -I$(SNETBASE)/include
LDFLAGS = -shared -lpthread -pthread

OBJS = entity.o stream.o streamset.o

TARGET = libtbpthread.so

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	gcc $(CFLAGS) $<

clean:
	rm -fr $(OBJS) $(TARGET)
