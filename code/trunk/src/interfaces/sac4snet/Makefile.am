

#MPIINCDIRS = -I$(MPIROOT)/include

IFNAME := SAC4SNet

WRAPLIBDIR := sac_lib
WRAPINCDIR := sac_inc

SAC2CFLAGS  = -v0

SACMODULE = $(IFNAME)FibreIO
S4CFLAGS  = -v0 -incdir $(WRAPINCDIR) -libdir $(WRAPLIBDIR) -o $(SACMODULE)
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACMODULE)`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACMODULE)`

SACMOD_LIB_NAMES := SNet $(IFNAME)InputGen $(SACMODULE)

SACMODLIBS := $(patsubst %,lib%Mod.so,$(SACMOD_LIB_NAMES))



pkglib_LTLIBRARIES = \
        libSAC4SNet.la \
        libSAC4SNetc.la

pkginclude_HEADERS = \
	SAC4SNet.h \
	SAC4SNetc.h


BUILT_SOURCES = \
        $(WRAPINCDIR)/$(IFNAME)FibreIO.h

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
libSAC4SNet_la_LIBADD = \
        $(SACMODLIBS)


# compiler plugin
libSAC4SNetc_la_SOURCES = \
	SAC4SNetc.c \
	SAC4SNetc.h
libSAC4SNetc_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/include
libSAC4SNetc_la_LIBADD = \
        -L$(top_srcdir) -lsnetutil



$(WRAPINCDIR)/$(IFNAME)FibreIO.h: $(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so

# these are built by sac2c
lib%Mod.so: %.sac
	$(AM_V_GEN) $(SAC2C) $(SAC2CFLAGS) $<

# the sac4c wrapper
$(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so: $(IFNAME)FibreIO.sac $(SACMODLIBS)
	$(AM_V_at)  mkdir -p $(WRAPINCDIR) $(WRAPLIBDIR)
	$(AM_V_GEN) $(SAC4C) $(S4CFLAGS) $(SACMODULE)

install-exec-hook:
	$(AM_V_at) echo installing sac4snet specific libraries
	$(AM_V_at) cp -p $(patsubst %,lib%Mod.so, $(SACMOD_LIB_NAMES))  \
                         $(patsubst %,lib%Tree.so, $(SACMOD_LIB_NAMES)) \
                         $(WRAPLIBDIR)/lib$(IFNAME)FibreIO.so \
                         $(pkglibdir)

uninstall-local:
	$(AM_V_at) echo uninstalling sac4snet specific libraries
	$(AM_V_at) rm -f $(patsubst %,$(pkglibdir)/%,$(patsubst %,lib%Mod.so, $(SACMOD_LIB_NAMES)))  \
                         $(patsubst %,$(pkglibdir)/%,$(patsubst %,lib%Tree.so, $(SACMOD_LIB_NAMES))) \
                         $(pkglibdir)/lib$(IFNAME)FibreIO.so

clean-local:
	-rm -f *.o *.a *.so
	-rm -fr $(WRAPINCDIR) $(WRAPLIBDIR)

