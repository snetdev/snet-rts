# build targets

if ENABLE_SAC4SNET
pkglib_LTLIBRARIES = \
        libSAC4SNet.la \
        libSAC4SNetc.la

pkginclude_HEADERS = \
	SAC4SNet.h \
	SAC4SNetc.h

SAC2CTARGET = lpel
SAC2CLIBVARIANT = -$(SAC2CTARGET)

SACMODLIBS  = libSNetMod$(SAC2CLIBVARIANT).so libSAC4SNetInputGenMod$(SAC2CLIBVARIANT).so libSAC4SNetFibreIOMod$(SAC2CLIBVARIANT).so
SACTREELIBS = libSNetTree$(SAC2CLIBVARIANT).so libSAC4SNetInputGenTree$(SAC2CLIBVARIANT).so libSAC4SNetFibreIOTree$(SAC2CLIBVARIANT).so

sacmoddir = $(pkglibdir)
sacmod_DATA = \
	 $(SACMODLIBS) $(SACMODLIBS:.so=.a) \
         $(SACTREELIBS) \
         $(WRAPLIBDIR)/libSAC4SNetFibreIO.so $(WRAPLIBDIR)/libSAC4SNetFibreIO.a

BUILT_SOURCES = \
        $(WRAPINCDIR)/SAC4SNetFibreIO.h

install-exec-hook:
	chmod +x $(pkglibdir)/*.so

endif

EXTRA_DIST = \
	SAC4SNetFibreIO.sac \
	SAC4SNetInputGen.sac \
	SNet.sac

### source code ###

# runtime interface library
libSAC4SNet_la_SOURCES = \
	SAC4SNet.c \
	SAC4SNet.h

libSAC4SNet_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include \
        -I$(SAC2CBASE)/include \
        -I$(SAC2CBASE)/src/include \
        $(S4CINCS)
if ENABLE_DIST_MPI
libSAC4SNet_la_CPPFLAGS += -I$(top_srcdir)/src/distrib/mpi
endif
if ENABLE_DIST_SCC
libSAC4SNet_la_CPPFLAGS += -I$(top_srcdir)/../../../scc-hg/rcce/include -I$(top_srcdir)/src/distrib/scc
endif

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

# you may optionally specify the -nophm in SAC2CFLAGS and S4CFLAGS to disable the private heap manager of sac.
SAC2CFLAGS  = -g -v0 -mt -ccflag"$(LPEL_INCLUDES)" -target $(SAC2CTARGET) -nophm
S4CFLAGS  = -g -v0 -mt -ccflag"$(LPEL_INCLUDES)" -target $(SAC2CTARGET) -incdir $(WRAPINCDIR) -libdir $(WRAPLIBDIR) -o SAC4SNetFibreIO -nophm

S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags SAC4SNetFibreIO`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags SAC4SNetFibreIO`

if ENABLE_SAC4SNET

# these are built by sac2c
lib%Mod$(SAC2CLIBVARIANT).so lib%Mod$(SAC2CLIBVARIANT).a lib%Tree$(SAC2CLIBVARIANT).so: %.sac
	$(AM_V_at) printf "[%s]" $(SAC2C)
	$(AM_V_GEN) $(SAC2C) $(SAC2CFLAGS) $<

# the sac4c wrapper
$(WRAPINCDIR)/SAC4SNetFibreIO.h $(WRAPLIBDIR)/libSAC4SNetFibreIO.a $(WRAPLIBDIR)/libSAC4SNetFibreIO.so: SAC4SNetFibreIO.sac $(SACMODLIBS) $(SACMODLIBS:.so=.a)
	$(AM_V_at)  $(MKDIR_P) $(WRAPINCDIR) $(WRAPLIBDIR)
	$(AM_V_at) printf "[%s]" $(SAC4C)
	$(AM_V_GEN) $(SAC4C) $(S4CFLAGS) -o SAC4SNetFibreIO SAC4SNetFibreIO

endif

DISTCLEANFILES = $(WRAPINCDIR)/*
CLEANFILES = $(SACMODLIBS) $(SACMODLIBS:.so=.a) $(SACTREELIBS)

clean-local:
	rm -rf $(WRAPINCDIR) $(WRAPLIBDIR)

