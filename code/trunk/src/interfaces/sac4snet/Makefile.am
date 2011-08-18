# build targets

if ENABLE_SAC4SNET
pkglib_LTLIBRARIES = \
        libSAC4SNet.la \
        libSAC4SNetc.la

pkginclude_HEADERS = \
	SAC4SNet.h \
	SAC4SNetc.h

SACMODLIBS  = libSNetMod.so libSAC4SNetInputGenMod.so libSAC4SNetFibreIOMod.so
SACTREELIBS = libSNetTree.so libSAC4SNetInputGenTree.so libSAC4SNetFibreIOTree.so

pkglib_DATA = \
	 $(SACMODLIBS) $(SACMODLIBS:.so=.a) \
         $(SACTREELIBS) \
         $(WRAPLIBDIR)/libSAC4SNetFibreIO.so $(WRAPLIBDIR)/libSAC4SNetFibreIO.a

endif

EXTRA_DIST = \
	SAC4SNetFibreIO.sac \
	SAC4SNetInputGen.sac \
	SNet.sac

### source code ###
BUILT_SOURCES = \
        $(WRAPINCDIR)/SAC4SNetFibreIO.h

# runtime interface library
libSAC4SNet_la_SOURCES = \
	SAC4SNet.c \
	SAC4SNet.h \
        $(WRAPINCDIR)/SAC4SNetFibreIO.h

libSAC4SNet_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include \
        -I$(SAC2CBASE)/include \
        -I$(SAC2CBASE)/src/include \
        $(S4CINCS)

libSAC4SNet_la_LDFLAGS = $(S4CLIBS)

libSAC4SNet_la_LIBADD = $(SACMODLIBS)

# compiler plugin
libSAC4SNetc_la_SOURCES = \
	SAC4SNetc.c \
	SAC4SNetc.h

libSAC4SNetc_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include

libSAC4SNetc_la_LIBADD = \
        -L$(top_srcdir) -lsnetutil

#### Build rules ####


WRAPLIBDIR = sac_lib
WRAPINCDIR = sac_inc

SAC2CFLAGS  = -v0
S4CFLAGS  = -v0 -incdir $(WRAPINCDIR) -libdir $(WRAPLIBDIR) -o SAC4SNetFibreIO

S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags SAC4SNetFibreIO`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags SAC4SNetFibreIO`

# these are built by sac2c
lib%Mod.so lib%Mod.a lib%Tree.so: %.sac
	$(AM_V_GEN) $(SAC2C) $(SAC2CFLAGS) $<

# the sac4c wrapper
$(WRAPINCDIR)/SAC4SNetFibreIO.h $(WRAPLIBDIR)/libSAC4SNetFibreIO.a $(WRAPLIBDIR)/libSAC4SNetFibreIO.so: SAC4SNetFibreIO.sac $(SACMODLIBS) $(SACMODLIBS:.so=.a)
	$(AM_V_at)  $(MKDIR_P) $(WRAPINCDIR) $(WRAPLIBDIR)
	$(AM_V_GEN) $(SAC4C) $(S4CFLAGS) -o SAC4SNetFibreIO SAC4SNetFibreIO

CLEANFILES = $(SACMODLIBS) $(SACMODLIBS:.so=.a) $(SACTREELIBS)


clean-local:
	rm -rf $(WRAPINCDIR) $(WRAPLIBDIR)

