

ifdef MPIMODE
DISTLIB_LA =  $(SNET_LIBS)/libdistribmpi.la
else
DISTLIB_LA =  $(SNET_LIBS)/libdistribnodist.la
endif

SAC4SNETLIBS := -L$(SNETBASE)/src/interfaces/sac4snet \
                # -L$(SNET_LIBS) \

BOXDIR := boxes

WRAPPERLIBDIR := lib
WRAPPERINCDIR := include

CCFLAGS   = -Wall -O2
INCDIRS   = -I. -I$(SNET_INCLUDES) -I./$(WRAPPERINCDIR)
LIBDIRS   = -L. -L$(BOXDIR) -L$(BOXDIR)/src \
            -L$(SNETBASE) \
            $(SAC4SNETLIBS) \
            -L./$(WRAPPERLIBDIR)


SACNAMES  = -DSACTYPE_SNet_SNet=23 -DSNetMain__$(TARGET)=main
TMAPIDS   = -DCID=24 -DCPXID=20 -DDISPID=26

SNETC      = snetc

# - - - - - - - - - - - - - - - - - - - -
SACTARGET = boxes
SACDISP   = SNetDisplay
SACFRAC   = Fractal_module
SACMANDEL = mandelbrot
# - - - - - - - - - - - - - - - - - - - -

SAC2C     = sac2c
S2CFLAGS  = -v1 -O3
SAC4C     = sac4c
S4CFLAGS  = -v0 \
            $(SAC4SNETLIBS) -L$(BOXDIR) \
            -incdir $(WRAPPERINCDIR) \
            -libdir $(WRAPPERLIBDIR) \

S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags -o $(SACTARGET) $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags -o $(SACTARGET) $(SACTARGET)`

LT = $(SNET_LIBS)/libtool

export SNET_EXTRA_CFLAGS = \
    -I$(SNETBASE)/src/interfaces/sac4snet \
    $(S4CINCS) \
    $(SACNAMES)

export SNET_EXTRA_LDFLAGS += $(LIBDIRS) $(S4CLIBS)


.PHONY: all clean

#########################################################
ifdef TARGET

all: $(TARGET)

else

all:
	@echo ""
	@echo "TARGET not defined (use 'make -f Makefile.xxx')"
	@echo ""
endif
#########################################################




$(BOXDIR)/lib$(SACTARGET)Mod.so:  $(BOXDIR)/$(SACTARGET).sac \
                                  $(BOXDIR)/lib$(SACDISP)Mod.so \
                                  $(BOXDIR)/lib$(SACMANDEL)Mod.so
	$(SAC2C) $(S2CFLAGS) $(TMAPIDS) $(LIBDIRS) \
            -o $(BOXDIR) $(BOXDIR)/$(SACTARGET).sac

$(WRAPPERINCDIR)/$(SACTARGET).h: $(BOXDIR)/lib$(SACTARGET)Mod.so
	mkdir -p $(WRAPPERINCDIR) $(WRAPPERLIBDIR)
	$(SAC4C) $(S4CFLAGS) $(SAC4SNETLIBS) -o $(SACTARGET) -L$(BOXDIR) $(SACTARGET)

$(BOXDIR)/lib$(SACFRAC)Mod.so: $(BOXDIR)/$(SACFRAC).sac
	$(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o $(BOXDIR) $(BOXDIR)/$(SACFRAC).sac

$(BOXDIR)/lib$(SACDISP)Mod.so: $(BOXDIR)/$(SACDISP).sac $(BOXDIR)/src/lib/src.a
	cd $(BOXDIR); $(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o . $(SACDISP).sac; cd ..;

$(BOXDIR)/src/lib/src.a:
	cd $(BOXDIR)/src; make; cd ../..;

$(BOXDIR)/lib$(SACMANDEL)Mod.so: $(BOXDIR)/$(SACMANDEL).sac \
                             $(BOXDIR)/lib$(SACFRAC)Mod.so
	$(SAC2C) $(TMAPIDS) $(S2CFLAGS) $(LIBDIRS) -o $(BOXDIR) $(BOXDIR)/$(SACMANDEL).sac

$(TARGET): $(TARGET).snet $(WRAPPERINCDIR)/$(SACTARGET).h
	LD_LIBRARY_PATH=$(SNETBASE)/src/interfaces/sac4snet:$(BOXDIR):$(LD_LIBRARY_PATH) && \
	  $(SNETC) $(SNETCFLAGS) -threading lpel    -o $@.lpel $<
	LD_LIBRARY_PATH=$(SNETBASE)/src/interfaces/sac4snet:$(BOXDIR):$(LD_LIBRARY_PATH) && \
	  $(SNETC) $(SNETCFLAGS) -threading pthread -o $@      $<

clean:
	$(LT) --mode=clean rm -f $(TARGET) $(TARGET).lpel $(TARGET).lo $(TARGET).[och]

allclean: clean
	make -C boxes/src clean
	rm -f *.o *.so *.a
	rm -f $(WRAPPERLIBDIR)/*.so $(WRAPPERLIBDIR)/*.a  $(WRAPPERINCDIR)/$(SACTARGET).h
	rm -f $(BOXDIR)/*.so $(BOXDIR)/*.a
