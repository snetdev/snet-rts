# DES Demo - Makefile for S-Net
#
# Frank Penczek (f.penczek@herts.ac.uk)
# Stephan Herhut (s.a.herhut@herts.ac.uk)
# Daniel Prokesch (d.prokesch@herts.ac.uk)
#

# --- general setup
CC         = gcc
CCFLAGS    = -Wall -g
SNETC	   = snetc
SNETCFLAGS = -b7 -v0
AR         = ar

# --- S-Net Runtime / Interface Paths and Libraries
INCDIRS   = -I. -I$(SNETBASE)/include -I./include # -I$(SACBASE)/sac2c/include
LIBDIRS   = -L. -L$(SNETBASE)/lib -L./boxes/libs
LIBS      = -lruntimestream -ldistribnodist -lsnetutil -lSAC4SNet -lpthread
LIBS_PTHR = -ltbpthread
LIBS_LPEL = -ltblpel

DEFINES   = -DSNetMain__simpleDes=main -DSACTYPE_SNet_SNet=21

# --- Include Directory for SAC Boxes
BOXINCS   = -I../boxes/include

SAC4C     = sac4c
S4CFLAGS  = -v3 -g $(LIBDIRS) -o simpleDesboxes
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)`

# = = = = = = = = = = = = = = = = = = = =
TARGET    = simpleDes
# = = = = = = = = = = = = = = = = = = = =
SACTARGET = simpleDesboxes
SACBOXLIB = boxes/libs/lib$(SACTARGET)Mod.so
# = = = = = = = = = = = = = = = = = = = =


.PHONY: all clean test # $(SACBOXLIB)

all: $(TARGET)

lpel: $(TARGET).lpel

$(TARGET): $(TARGET).o $(SACBOXLIB)
	$(CC) $(LIBDIRS) $(RPATH) -o $@ $< $(S4CLIBS) $(LIBS) $(LIBS_PTHR)

$(TARGET).lpel: $(TARGET).o $(SACBOXLIB)
	$(CC) $(LIBDIRS) $(RPATH) -o $@ $< $(S4CLIBS) $(LIBS) $(LIBS_LPEL)


$(TARGET).o: $(TARGET).c
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(DEFINES) -c $<

$(TARGET).c: $(TARGET).snet
	echo $(SNETCFLAGS)
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet

$(SACBOXLIB): boxes/simpleDesboxes.sac
	make -C boxes/

test: $(TARGET)
	./$(TARGET) < input.xml

clean:
	make -C boxes/ clean
	rm -f  $(TARGET) $(TARGET).lpel $(TARGET).[och]
	rm -f  mon_*.log n*tasks.map

