#
# create a shared library
#
# relies on the environment variable LPELSOURCE,
# which points to a directory containing the lpel
# sources from the external lpel directory
#

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

%.o: $(LPELSOURCE)/%.c lpel4snet.h translate4snet.sed
	gcc -E $< |              \
	sed -f translate4snet.sed |\
	gcc -o $@ $(CFLAGS) -xc -

%.o: %.c lpel4snet.h translate4snet.sed
	gcc -E $(INCLUDES) $< |              \
	sed -f translate4snet.sed |\
	gcc -o $@ $(CFLAGS) -xc -

lpel4snet.h: $(LPELSOURCE)/lpel.h translate4snet.sed
	sed -f translate4snet.sed \
	    -e "/typedef .*snet_.*/ d" \
	    $< > $@

clean:
	rm -fr lpel4snet.h $(OBJS) $(TARGET)
