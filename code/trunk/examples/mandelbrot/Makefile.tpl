include $(SNETBASE)/src/makefiles/config.mkf


ifdef MPIMODE
CC        = mpicc
LANGIFLIB = -lSAC4SNetMPI
DISTLIB   =  -ldistribmpi
else
CC        = gcc
LANGIFLIB = -lSAC4SNet
DISTLIB   = -ldistribnodist
endif

COMMONLIB = $(LANGIFLIB) $(DISTLIB) -lsnetutil -lruntimestream
TBLPELLIB = -ltblpel
TBPTLIB   = -lpthread -ltbpthread

CCFLAGS   = -Wall -O3
AR        = ar
INCDIRS   = -I. -I$(SNETBASE)/include -I./include
LIBDIRS   = -L. -Lboxes -Lboxes/src -L$(SNETBASE)/lib -L./lib


SACNAMES  = -DSACTYPE_SNet_SNet=23 -DSNetMain__$(TARGET)=main
TMAPIDS   = -DCID=24 -DCPXID=20 -DDISPID=26

SNETC      = snetc
SNETCFLAGS = -b7 -v0 # linking is done separately

# - - - - - - - - - - - - - - - - - - - -
SACTARGET = boxes
SACDISP   = SNetDisplay
SACFRAC   = Fractal_module
SACMANDEL = mandelbrot
# - - - - - - - - - - - - - - - - - - - -

SAC2C     = sac2c
S2CFLAGS  = -v0 -O3
SAC4C     = sac4c
S4CFLAGS  = -v3 -incdir include -libdir lib $(LIBDIRS)
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags -o $(SACTARGET) $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags -o $(SACTARGET) $(SACTARGET)`



.PHONY: all clean

ifdef TARGET

all:  include/$(SACTARGET).h $(TARGET).c
	$(CC) $(CCFLAGS) $(INCDIRS) $(S4CINCS) $(SACNAMES) -c $(TARGET).c
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET) $(TARGET).o $(S4CLIBS) $(COMMONLIB) $(TBPTLIB)
	$(CC) $(LIBDIRS) $(RPATH) -o $(TARGET).lpel $(TARGET).o $(S4CLIBS) $(COMMONLIB) $(TBLPELLIB)

else

all:
	@echo ""
	@echo "TARGET not defined (use 'make -f Makefile.xxx')"
	@echo ""
endif

boxes/lib$(SACTARGET)Mod.so: boxes/$(SACTARGET).sac \
                             boxes/lib$(SACDISP)Mod.so \
                             boxes/lib$(SACMANDEL)Mod.so 
	$(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o boxes boxes/$(SACTARGET).sac

include/$(SACTARGET).h: boxes/lib$(SACTARGET)Mod.so 
	mkdir -p include lib
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) -o $(SACTARGET) -Lboxes $(SACTARGET)
  
boxes/lib$(SACFRAC)Mod.so: boxes/$(SACFRAC).sac
	$(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o boxes boxes/$(SACFRAC).sac

boxes/lib$(SACDISP)Mod.so: boxes/$(SACDISP).sac boxes/src/lib/src.a
	cd boxes; $(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o . $(SACDISP).sac; cd ..;

boxes/src/lib/src.a:
	cd boxes/src; make; cd ../..;

boxes/lib$(SACMANDEL)Mod.so: boxes/$(SACMANDEL).sac \
                             boxes/lib$(SACFRAC)Mod.so 
	$(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o boxes boxes/$(SACMANDEL).sac

$(TARGET).c: $(TARGET).snet
	$(SNETC) $(SNETCFLAGS) $(TARGET).snet

clean:
	rm -f  $(TARGET) $(TARGET).[oach] 

allclean:
	cd boxes/src; make clean; cd ../..
	rm -f *.o *.so *.a  lib/*.so lib/*.a  include/$(SACTARGET).h $(TARGET) $(TARGET).? boxes/*.so boxes/*.a
