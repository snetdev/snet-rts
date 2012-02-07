include $(SNETBASE)/src/makefiles/config.mkf

NAME       = C4SNet

CC         = gcc
CCFLAGS    = -Wall -g -fPIC
AR         = ar
INCDIRS    = -I. -I$(SNETBASE)/include
COPY       = cp

# = = = = = = = = = = = = = = = = = = = =
OBJECTS     = $(NAME).o base64.o 
TARGET      = lib$(NAME).so 
WRAPOBJECTS = $(NAME)c.o
WRAPTARGET  = lib$(NAME)c.so
# = = = = = = = = = = = = = = = = = = = =
TARGETLIBDIR   = $(SNETBASE)/lib
TARGETINCDIR   = $(SNETBASE)/include
# = = = = = = = = = = = = = = = = = = = =


all: default

default:
	make clean
	$(CC) $(CCFLAGS) $(INCDIRS) -c *.c
	$(CC) $(subst %libname%,$(notdir $(TARGET)),$(LD_DYNAMIC))\
  -o $(TARGET) $(OBJECTS) 
	$(CC) $(subst %libname%,$(notdir $(WRAPTARGET)),$(LD_DYNAMIC))\
  -o $(WRAPTARGET) $(WRAPOBJECTS) 
	$(COPY) $(TARGET) $(WRAPTARGET) $(TARGETLIBDIR)
	$(COPY) $(NAME).h $(TARGETINCDIR)

clean:
	rm -f *.o *.a *.so $(TARGETLIBDIR)/$(TARGET)\
  $(TARGETLIBDIR)/$(WRAPTARGET) $(TARGETINCDIR)/$(NAME).h
