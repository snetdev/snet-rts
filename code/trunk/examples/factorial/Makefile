include $(SNETBASE)/src/makefiles/config.mkf

# = = = = = = = = = = = = = = = = = = = =
TARGET    = factorial
BOXES     = boxes/libboxes.a
# = = = = = = = = = = = = = = = = = = = =

SNETC     = snetc
LIBS      = $(SOCKETLIBS) $(RPATH)

.PHONY: all clean test

all: $(TARGET) $(TARGET).lpel

$(TARGET): $(TARGET).snet $(BOXES)
	$(SNETC) -g $(LIBS) -o $(TARGET) $(TARGET).snet -L./boxes -lboxes

$(TARGET).lpel: $(TARGET).snet $(BOXES)
	$(SNETC) -g $(LIBS) -o $(TARGET).lpel $(TARGET).snet -L./boxes -lboxes -threading lpel

$(BOXES):
	make -C boxes/

test: $(TARGET)
	./$(TARGET) < data/data.xml
clean:
	make -C boxes/ clean
	rm -f $(TARGET) $(TARGET).lpel $(TARGET).[och]
	rm -f mon*.log n*_tasks.map
