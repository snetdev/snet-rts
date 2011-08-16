

#MPIINCDIRS = -I$(MPIROOT)/include

IFNAME := SAC4SNet

WRAPLIBDIR  := sac_lib
WRAPINCDIR  := sac_inc

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


# runtime interface library
libSAC4SNet_la_SOURCES = \
	SAC4SNet.c \
	SAC4SNet.h \
        sacinterface.h \
        $(WRAPINCDIR)/$(IFNAME)FibreIO.h
libSAC4SNet_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include \
        -I$(SAC2CBASE)/include \
        -I$(SAC2CBASE)/src/include \
        $(S4CINCS)
libSAC4SNet_la_LDFLAGS = \
        $(S4CLIBS)


# compiler plugin
libSAC4SNetc_la_SOURCES = \
	SAC4SNetc.c \
	SAC4SNetc.h
libSAC4SNetc_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include
libSAC4SNetc_la_LIBADD = \
        -L$(top_srcdir) -lsnetutil




sacmodules: libSNetMod.so libSAC4SNetInputGenMod.so $(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so

$(WRAPINCDIR)/$(IFNAME)FibreIO.h: sacmodules

# these are built by sac2c
lib%Mod.so: %.sac
	$(AM_V_GEN) $(SAC2C) $(SAC2CFLAGS) $<

# the sac4c wrapper
$(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so: lib$(SACTARGET)Mod.so $(IFNAME)FibreIO.sac
	$(AM_V_at)mkdir -p $(WRAPINCDIR) $(WRAPLIBDIR)
	$(AM_V_GEN) $(SAC4C) $(S4CFLAGS) $(SACTARGET)




clean-local:
	-rm -f *.o *.a *.so
	-rm -fr $(WRAPINCDIR) $(WRAPLIBDIR)
