include ../../paths.mkf

TARGET	:=	pipe

BOXES   := boxA boxB
BOXPATH := ./boxes

.PHONY: clean

all: $(TARGET) 


$(TARGET): $(TARGET).snet lib$(TARGET)Boxes.a
	snetc $(INCLUDES) \
            -L. -L$(BOXPATH) -L$(SNETBASE)/lib \
            -l$(TARGET)Boxes -lm $(LIBRARIES) \
            $(TARGET).snet -o $(TARGET)


lib$(TARGET)Boxes.a: $(patsubst %, $(BOXPATH)/%.o, $(BOXES))
	ar -r lib$(TARGET)Boxes.a $(BOXPATH)/*.o 

%.o: %.c %.h
	gcc -Wall -pedantic $(INCLUDES) -c $< -o $@



clean:
	- rm -f $(TARGET).[ch] *.o $(BOXPATH)/*.o lib$(TARGET)Boxes.a
	- libtool --mode=clean rm -f $(TARGET) $(TARGET).lo
