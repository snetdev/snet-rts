include ../../paths.mkf

TARGET = innerroot
SOURCES = boxes.c
OBJECTS = $(patsubst %.c,%.lo,$(SOURCES))
INCLUDES = -I$(SNET_INCLUDES)
SNETTESTFLAGS = -g -v1 -q $(INCLUDES)
FRONT = -distrib mpi -threading front
PTHRD = -distrib mpi -threading pthread
CFLAGS += -g -Wall $(INCLUDES)

export SNET_EXTRA_LDFLAGS+=$(BOXES)

.PHONY: clean all lpel

all: $(TARGET) $(TARGET).pth tell tell.pth

$(TARGET):  $(TARGET).snet $(BOXES) $(SOURCES)
	snetc $(SNETCFLAGS) $(FRONT) $< -o $@ $(SOURCES)

$(TARGET).pth: $(TARGET).snet
	snetc $(SNETCFLAGS) $(PTHRD) $< -o $@ $(OBJECTS)

tell: tell.snet $(OBJECTS)
	snetc $(SNETCFLAGS) $(FRONT) $< -o $@ $(OBJECTS)

tell.pth: tell.snet $(OBJECTS)
	snetc $(SNETCFLAGS) $(PTHRD) $< -o $@ $(OBJECTS)

clean:
	$(RM) $(RMFLAGS) -- $(TARGET) $(TARGET).pth tell tell.pth $(TARGET).[och] *.o *.lo core* vgcore.*

