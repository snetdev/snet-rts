#
# create a shared library
#
# relies on the environment variable LPELSOURCE,
# which points to a directory containing the lpel
# sources from the external lpel directory
#

INCLUDES = -I$(SNETBASE)/include -I$(LPELSOURCE) 
CFLAGS = -g -Wall -pthread -DUSE_MCTX_PCL -fPIC $(INCLUDES)
LDFLAGS = -undefined suppress -flat_namespace -dynamiclib 

OBJS = glue_snet.o assign.o mon_snet.o 

TARGET = libtblpel.so



all: $(TARGET)
	cp $(TARGET) $(SNETBASE)/lib

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -fr $(OBJS) $(TARGET)
