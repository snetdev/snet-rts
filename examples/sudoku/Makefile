include $(SNETBASE)/src/makefiles/config.mkf

CC        = gcc
CCFLAGS   = -Wall -g
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include  -I./boxes/include
LIBDIRS   = -L. -L$(SNETBASE)/lib  -L./boxes/lib -Lboxes
LIBS      = -lruntimestream -ldistribnodist -lsnetutil -lSAC4SNet -lpthread
LIBS_PTHR = -ltbpthread
LIBS_LPEL = -ltblpel

SACNAMES   = -DSACTYPE_SNet_SNet=20 -DSNetMain__sudoku=main

# SAC SimpleFibre library
# FIBREOBJS = $(SACBASE)/stdlib/world/stdio/src/FibreIO/*.o


SNETC	  = snetc
SNETCFLAGS= -b7


# - - - - - - - - - - - - - - - - - - - -
SACTARGET = sudoku
SACBOXLIB = boxes/lib$(SACTARGET)Mod.so
# - - - - - - - - - - - - - - - - - - - -

SAC4C     = sac4c
S4CFLAGS  = -v3 -g $(LIBDIRS) -o boxes
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)`


# = = = = = = = = = = = = = = = = = = = =
TARGET    = sudoku
# = = = = = = = = = = = = = = = = = = = =


.PHONY: all clean test $(SACBOXLIB)



all: $(TARGET) $(TARGET).lpel

$(TARGET): $(TARGET).o $(SACBOXLIB)
	$(CC) $(LIBDIRS) $(RPATH) -o $@ $< $(S4CLIBS) $(LIBS) $(LIBS_PTHR)

$(TARGET).lpel: $(TARGET).o $(SACBOXLIB)
	$(CC) $(LIBDIRS) $(RPATH) -o $@ $< $(S4CLIBS) $(LIBS) $(LIBS_LPEL)


$(TARGET).o: $(TARGET).c
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(SACNAMES) -c $<

$(TARGET).c: $(TARGET).snet
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet

$(SACBOXLIB):
	make -C boxes/

test: $(TARGET)
	./$(TARGET) < input.xml

clean:
	make -C boxes/ clean
	rm -f  $(TARGET) $(TARGET).lpel $(TARGET).[och]
	rm -f  mon_*.log n*tasks.map



