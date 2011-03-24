#
# create a shared library
#

# relies on the environment variable LPELSOURCE

INCLUDES = -I$(SNETBASE)/include -I$(LPELSOURCE)
CFLAGS = -c -g -Wall -pthread -fPIC $(INCLUDES)
LDFLAGS = -shared -pthread -lpthread -lpcl -lrt -lcap

OBJS = buffer.o monitoring.o scheduler.o stream.o \
       streamset.o task.o taskqueue.o lpel_main.o worker.o \
       glue_snet.o assign.o \
       mailbox.o \
       # mailbox-lf.o \

TARGET = libtblpel.so



all: $(TARGET)
	cp $(TARGET) $(SNETBASE)/lib

$(TARGET): $(OBJS)
	gcc $(LDFLAGS) -o $@ $(OBJS)

%.o: $(LPELSOURCE)/%.c
	gcc $(CFLAGS) $<

clean:
	rm -fr $(OBJS) $(TARGET)
