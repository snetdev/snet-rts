###### Common definitions ######

sacmoddir = $(pkglibdir)
pkginclude_HEADERS =
nobase_sacmod_DATA =
nobase_pkglib_LTLIBRARIES =
BUILT_SOURCES =
EXTRA_DIST =

# runtime interface library
AM_CPPFLAGS = -I$(top_srcdir)/include
if ENABLE_DIST_MPI
AM_CPPFLAGS += -I$(top_srcdir)/src/distrib/mpi
endif
if ENABLE_DIST_SCC
AM_CPPFLAGS += -I$(top_srcdir)/../../../scc-hg/rcce/include -I$(top_srcdir)/src/distrib/scc
endif

if ENABLE_SAC4SNET

###### Compiler plugin ######

pkginclude_HEADERS += SAC4SNetc.h
nobase_pkglib_LTLIBRARIES += libSAC4SNetc.la

libSAC4SNetc_la_SOURCES = SAC4SNetc.c SAC4SNetc.h
libSAC4SNetc_la_LIBADD = -L$(top_builddir) -lsnetutil

###### SAC4SNet modules ######

SACMODULES = SAC4SNetFibreIO.sac SAC4SNetInputGen.sac SNet.sac
SAC4SNET_COMMON_SRC = SAC4SNet.c SAC4SNet.h
SAC4SNET_COMMON_PREBUILD = SAC4SNetFibreIO.s4c_cppflags SAC4SNetFibreIO.s4c_ldflags $(SACMODULES:%.sac=%.s2c_done)

pkginclude_HEADERS += SAC4SNet.h
EXTRA_DIST += $(SACMODULES)
TARGET_DIRS =

if ENABLE_SAC4SNET_SEQ
TARGET_DIRS += seq
nobase_pkglib_LTLIBRARIES += seq/libSAC4SNet.la
nobase_sacmod_DATA += seq/*.so seq/*.a
BUILT_SOURCES += $(SAC4SNET_COMMON_PREBUILD:%=seq/%)
endif
if ENABLE_SAC4SNET_MT_PTH
TARGET_DIRS += mtpth
nobase_pkglib_LTLIBRARIES += mtpth/libSAC4SNet.la
nobase_sacmod_DATA += mtpth/*.so mtpth/*.a
BUILT_SOURCES += $(SAC4SNET_COMMON_PREBUILD:%=mtpth/%)
endif
if ENABLE_SAC4SNET_MT_LPEL
TARGET_DIRS += mtlpel
nobase_pkglib_LTLIBRARIES += mtlpel/libSAC4SNet.la
nobase_sacmod_DATA += mtlpel/*.so mtlpel/*.a
BUILT_SOURCES += $(SAC4SNET_COMMON_PREBUILD:%=mtlpel/%)
endif

### build rules ###
SAC4SNET_CPPFLAGS = `cat $(firstword $(subst _, ,$@))/SAC4SNetFibreIO.s4c_cppflags`
SAC4SNET_LDFLAGS = `cat $(@D)/SAC4SNetFibreIO.s4c_ldflags` $(patsubst $(@D)/lib%.a,-l%,$(wildcard $(@D)/lib*Mod*.a))

seq_libSAC4SNet_la_SOURCES = $(SAC4SNET_COMMON_SRC)
seq_libSAC4SNet_la_CPPFLAGS = $(AM_CPPFLAGS) $(SAC4SNET_CPPFLAGS)
seq_libSAC4SNet_la_LDFLAGS = $(AM_LDFLAGS) $(SAC4SNET_LDFLAGS)
mtpth_libSAC4SNet_la_SOURCES = $(SAC4SNET_COMMON_SRC)
mtpth_libSAC4SNet_la_CPPFLAGS = $(AM_CPPFLAGS) $(SAC4SNET_CPPFLAGS)
mtpth_libSAC4SNet_la_LDFLAGS = $(AM_LDFLAGS) $(SAC4SNET_LDFLAGS)
mtlpel_libSAC4SNet_la_SOURCES = $(SAC4SNET_COMMON_SRC)
mtlpel_libSAC4SNet_la_CPPFLAGS = $(AM_CPPFLAGS) $(SAC4SNET_CPPFLAGS)
mtlpel_libSAC4SNet_la_LDFLAGS = $(AM_LDFLAGS) $(SAC4SNET_LDFLAGS)

install-data-hook:
	find $(DESTDIR)$(pkglibdir) -name \*.so -exec chmod +x '{}' \;

clean-local:
	rm -rf $(TARGET_DIRS)

###### SAC-related rules ######

### common SAC2C/SAC4C flags
SAC2CFLAGS = -g -v0
SAC4CFLAGS = $(SAC2CFLAGS) -incdir $(@D) -libdir $(@D) -L $(@D) -o $(*F)

### SAC flags per target
SACFLAGS_seq = # nothing special
SACFLAGS_mtlpel = -mt -nophm -target lpel -ccflag"$(LPEL_INCLUDES)"
SACFLAGS_mtpth = -mt

SUFFIXES = s4c_done s2c_done s4c_ldflags s4c_cppflags sac

.SECONDARY: %.s4c_done %.s2c_done %.s4c_ldflags %.s4c_cppflags

%.s4c_done: %.s2c_done
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)$(SAC4C) $(SACFLAGS_$(*D)) $(SAC4CFLAGS) $(*F)
	$(AM_V_at)touch $@

%.s4c_ldflags: %.s4c_done
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)$(SAC4C) $(SACFLAGS_$(*D)) $(SAC4CFLAGS) -ldflags $(*F) >$@.tmp
	$(AM_V_at)mv $@.tmp $@

%.s4c_cppflags: %.s4c_done
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)$(SAC4C) $(SACFLAGS_$(*D)) $(SAC4CFLAGS) -ccflags $(*F) >$@.tmp
	$(AM_V_at)echo " -I$(SAC2CBASE)/include -I$(SAC2CBASE)/src/include" >>$@.tmp
	$(AM_V_at)mv $@.tmp $@


.SECONDEXPANSION:
# "SECONDEXPANSION" enables using delayed evaluation of make variables, eg $$(*F) here.
%.s2c_done: $$(*F).sac
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)$(SAC2C) $(SAC2CFLAGS) $(SACFLAGS_$(*D)) -o $(@D) $<
	$(AM_V_at)touch $@

endif
