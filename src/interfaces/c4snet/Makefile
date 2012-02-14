NAME       = C4SNet

CC         = gcc
CCFLAGS    = -Wall -g -fPIC
AR         = ar
INCDIRS    = -I. -I$(SNET_INCLUDES)
COPY       = cp

# = = = = = = = = = = = = = = = = = = = =
OBJECTS     = $(NAME).o base64.o 
TARGET      = lib$(NAME).so 
WRAPOBJECTS = $(NAME)c.o
WRAPTARGET  = lib$(NAME)c.so
# = = = = = = = = = = = = = = = = = = = =

all: default

default:
	make clean
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(subst %libname%,$(notdir $(TARGET)),$(LD_DYNAMIC))\
  -o $(TARGET) $(OBJECTS) 
	$(CC) $(subst %libname%,$(notdir $(WRAPTARGET)),$(LD_DYNAMIC))\
  -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(SNET_LIB)
	$(COPY) $(NAME).h $(SNET_INCLUDES)

clean:
	rm -f *.o *.a *.so $(SNET_LIBS)/$(TARGET)\
  $(SNET_LIBS)/$(WRAPTARGET) $(SNET_INCLUDES)/$(NAME).h
