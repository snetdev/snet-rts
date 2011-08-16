

#MPIINCDIRS = -I$(MPIROOT)/include

IFNAME := SAC4SNet

WRAPLIBDIR  = sac_lib
WRAPINCDIR  = sac_inc

SAC2CFLAGS  = -v0

SACTARGET = $(IFNAME)FibreIO
S4CFLAGS  = -v0 -incdir $(WRAPINCDIR) -libdir $(WRAPLIBDIR) -o $(SACTARGET)
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACTARGET)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACTARGET)`



.PHONY: sacmodules


pkglib_LTLIBRARIES = \
        libSAC4SNet.la \
        libSAC4SNetc.la

pkginclude_HEADERS = \
	SAC4SNet.h \
	SAC4SNetc.h

libSAC4SNet_la_SOURCES = \
	SAC4SNet.c \
	SAC4SNet.h \
        sacinterface.h \
        $(WRAPINCDIR)/SAC4SNetFibreIO.h
libSAC4SNet_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include \
        -I$(SAC2CBASE)/include \
        -I$(SAC2CBASE)/src/include \
        $(S4CINCS)
        #-I$(srcdir)/sac_include
libSAC4SNet_la_LDFLAGS = \
        $(S4CLIBS)

libSAC4SNetc_la_SOURCES = \
	SAC4SNetc.c \
	SAC4SNetc.h
libSAC4SNetc_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include \
       #-I$(SAC2CBASE)/include \
       #-I$(SAC2CBASE)/src/include
libSAC4SNetc_la_LIBADD = \
        -L$(top_srcdir) -lsnetutil




sacmodules: libSNetMod.so libSAC4SNetInputGenMod.so $(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so


# these are built by sac2c
lib%Mod.so: %.sac
	$(SAC2C) $(SAC2CFLAGS) $<

$(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so: $(IFNAME)FibreIO.sac
	$(SAC2C) $(SACFLAGS) $(SACTARGET).sac
	mkdir -p $(WRAPINCDIR) $(WRAPLIBDIR)
	$(SAC4C) $(S4CFLAGS) $(SACTARGET)

$(WRAPINCDIR)/SAC4SNetFibreIO.h: sacmodules

clean-local:
	-rm -f *.o *.a *.so
	-rm -fr $(WRAPINCDIR) $(WRAPLIBDIR)
