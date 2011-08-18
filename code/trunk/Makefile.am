
SUBDIRS = . src/interfaces/sac4snet

# stuff that gets installed
pkglib_DATA = \
	compile_cmd link_cmd \
	libtool

pkglib_LTLIBRARIES = \
	libsnetutil.la  \
	libruntimestream.la \
	libC4SNet.la \
	libC4SNetc.la \
	libtbpthread.la \
	libdistribnodist.la

pkginclude_HEADERS = \
	include/C4SNet.h \
	include/C4SNetc.h \
	include/atomiccnt.h \
	include/bool.h \
	include/constants.h \
	include/debug.h \
	include/distribution.h \
	include/entities.h \
	include/expression.h \
	include/filter.h \
	include/handle.h \
	include/info.h \
	include/interface.h \
	include/interface_functions.h \
	include/label.h \
	include/list-template.h \
	include/list-template.c \
	include/list.h \
	include/locvec.h \
	include/map-template.h \
	include/map-template.c \
	include/map.h \
	include/memfun.h \
	include/metadata.h \
	include/moninfo.h \
	include/networkinterface.h \
	include/out.h \
	include/record.h \
	include/snetentities.h \
	include/snettypes.h \
	include/stream.h \
	include/threading.h \
	include/type.h \
	include/variant.h


# How to build the stuff

libsnetutil_la_SOURCES = \
	src/util/core/memfun.c \
	src/util/metadata/metadata.c 

libsnetutil_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/src/include

libruntimestream_la_SOURCES = \
	src/runtime/common/expression.c \
	src/runtime/common/info.c \
	src/runtime/common/list.c \
	src/runtime/common/map.c \
	src/runtime/common/moninfo.c \
	src/runtime/common/record.c \
	src/runtime/common/variant.c \
        src/runtime/stream/utils/bitmap.c \
        src/runtime/stream/utils/bitmap.h \
        src/runtime/stream/utils/debug.c \
        src/runtime/stream/utils/debugcounters.c \
        src/runtime/stream/utils/debugcounters.h \
        src/runtime/stream/utils/debugtime.c \
        src/runtime/stream/utils/debugtime.h \
        src/runtime/stream/utils/hashtab.c \
        src/runtime/stream/utils/hashtab.h \
        src/runtime/stream/utils/hashtable.c \
        src/runtime/stream/utils/hashtable.h \
        src/runtime/stream/utils/locvec.c \
        src/runtime/stream/utils/queue.c \
        src/runtime/stream/utils/queue.h \
        src/runtime/stream/shared/handle.c \
        src/runtime/stream/shared/handle_p.h \
        src/runtime/stream/entity/alias.c \
        src/runtime/stream/entity/box.c \
        src/runtime/stream/entity/collector.c \
        src/runtime/stream/entity/collector.h \
        src/runtime/stream/entity/entities.c \
        src/runtime/stream/entity/feedback.c \
        src/runtime/stream/entity/filter.c \
        src/runtime/stream/entity/out.c \
        src/runtime/stream/entity/parallel.c \
        src/runtime/stream/entity/serial.c \
        src/runtime/stream/entity/split.c \
        src/runtime/stream/entity/star.c \
        src/runtime/stream/entity/syncro.c \
	src/runtime/stream/observer/observers.c \
	src/runtime/stream/observer/observers.h \
	src/runtime/stream/langif/interface_functions.c \
	src/runtime/stream/netif/interface.c \
	src/runtime/stream/netif/input.c \
	src/runtime/stream/netif/input.h \
	src/runtime/stream/netif/output.c \
	src/runtime/stream/netif/output.h \
	src/runtime/stream/netif/label.c \
	src/runtime/stream/netif/networkinterface.c \
	src/runtime/stream/netif/parserutils.h \
	src/runtime/stream/netif/parser.y \
	src/runtime/stream/netif/lexer.l
libruntimestream_la_CPPFLAGS = \
	-I$(srcdir)/src/runtime/stream/utils \
	-I$(srcdir)/src/runtime/stream/shared \
	-I$(srcdir)/src/runtime/stream/observer \
	-I$(srcdir)/src/runtime/stream/netif 

# for runtime/stream/netif
BUILT_SOURCES = src/runtime/stream/netif/parser.h
AM_YFLAGS = -d

libtbpthread_la_SOURCES = \
	src/threading/pthread/tbpth/entity.c \
	src/threading/pthread/tbpth/entity.h \
	src/threading/pthread/tbpth/stream.c \
	src/threading/pthread/tbpth/stream.h \
	src/threading/pthread/tbpth/streamset.c \
	src/threading/pthread/tbpth/monitorer.c \
	src/threading/pthread/tbpth/monitorer.h

if ENABLE_TH_LPEL
pkglib_LTLIBRARIES += libtblpel.la 
libtblpel_la_SOURCES = \
	src/threading/lpel/lpelmod/assign.c \
	src/threading/lpel/lpelmod/assign.h \
	src/threading/lpel/lpelmod/mon_snet.c \
	src/threading/lpel/lpelmod/mon_snet.h \
	src/threading/lpel/lpelif/glue_snet.c \
	src/threading/lpel/lpelif/wrappers.c 
libtblpel_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        $(LPEL_INCLUDES) \
        -I$(srcdir)/src/threading/lpel/lpelmod
libtblpel_la_LIBADD = $(LIBLPEL_LA)
endif

libC4SNet_la_SOURCES = \
	src/interfaces/c4snet/C4SNet.c \
	src/interfaces/c4snet/C4SNet.h \
	src/interfaces/c4snet/base64.h \
	src/interfaces/c4snet/base64.c

libC4SNetc_la_SOURCES = \
	src/interfaces/c4snet/C4SNetc.c \
	src/interfaces/c4snet/C4SNetc.h


libdistribnodist_la_SOURCES = \
	src/distrib/nodist/distribution.c

if ENABLE_DIST_MPI
pkglib_LTLIBRARIES += libdistribmpi.la
libdistribmpi_la_SOURCES = \
	src/distrib/common/imanager.c \
	src/distrib/common/omanager.c \
	src/distrib/mpi/distribmap.c \
	src/distrib/mpi/distribmap.h \
	src/distrib/mpi/distribution.c
libdistribmpi_la_CPPFLAGS = \
	-I$(srcdir)/src/distrib/mpi
endif

# common flags
INCLUDES = -I$(top_srcdir)/include

compile_cmd: Makefile
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)echo $(SHELL) '$${SNET_LIBS:-$$SNETBASE/lib}'/libtool --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile \
	     $(CC) $(DEFS) $(AM_CPPFLAGS) -I'$${SNET_INCLUDES:-$$SNETBASE/include}' $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c >$@

link_cmd: Makefile
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)echo $(SHELL) '$${SNET_LIBS:-$$SNETBASE/lib}'/libtool --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link \
	     $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -L'$${SNET_LIBS:-$$SNETBASE/lib}' >$@

# re-generate the "libtool" script when it becomes deprecated
LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

# ensure that aclocal finds the additional macros
ACLOCAL_AMFLAGS = -I build-aux

