
TARGET	:=	pipe

BOXES   := boxA boxB
BOXPATH := ./boxes

.PHONY: clean

all: $(TARGET) 


$(TARGET): $(TARGET).snet lib$(TARGET)Boxes.a 
			snetc -Wl,-rpath=$(SNETBASE)/lib  \
            -I. -I$(SNETBASE)/include \
            -L. -L$(BOXPATH) -L$(SNETBASE)/lib \
            -l$(TARGET)Boxes -lm \
            $(TARGET).snet -o $(TARGET)


lib$(TARGET)Boxes.a: $(patsubst %, $(BOXPATH)/%.o, $(BOXES))
	ar -r lib$(TARGET)Boxes.a $(BOXPATH)/*.o 

%.o: %.c %.h
			gcc -Wall -pedantic -I. -I$(SNETBASE)/include -c $< -o $@



clean:
			- rm -f $(TARGET) $(TARGET).[ch] *.o $(BOXPATH)/*.o lib$(TARGET)Boxes.a
