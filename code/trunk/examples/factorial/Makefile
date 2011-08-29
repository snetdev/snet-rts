INCLUDES  = -I.

ifdef SNET_INCLUDES
INCLUDES += -I$(SNET_INCLUDES)
endif

# following should go at some point in time
INCLUDES += -I$(SNETBASE)/include
INCLUDES += -I$(SNETBASE)/src/interfaces/c4snet
INCLUDES += -I$(SNETBASE)/src/interfaces/sac4snet

export INCLUDES

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
BOXES     = boxes/libboxes.a
# = = = = = = = = = = = = = = = = = = = =

SNETC     = snetc -distrib mpi
LIBS      = $(SOCKETLIBS) $(RPATH)

.PHONY: all clean test

all: $(TARGET) $(TARGET).lpel

$(TARGET): $(TARGET).snet $(BOXES)
	$(SNETC) -g $(LIBS) $(INCLUDES) -o $(TARGET) $(TARGET).snet -L./boxes -lboxes

$(TARGET).lpel: $(TARGET).snet $(BOXES)
	$(SNETC) -g $(LIBS) $(INCLUDES) -o $(TARGET).lpel $(TARGET).snet -L./boxes -lboxes -threading lpel

$(BOXES):
	make -C boxes/

test: $(TARGET)
	./$(TARGET) < data/data.xml
clean:
	make -C boxes/ clean
	rm -f $(TARGET) $(TARGET).lpel $(TARGET).[och]
	rm -f mon*.log n*_tasks.map
