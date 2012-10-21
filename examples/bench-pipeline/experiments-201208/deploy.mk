MKDIR_P = mkdir -p
CC = gcc
CFLAGS = 
CPPFLAGS = -I/scratch/opt/include -I/opt/arch/include
LDFLAGS = -L/scratch/opt/lib -L/opt/arch/lib
SVN = svn
GIT = git
GREP = grep
AUTORECONF = autoreconf

PREFIX=/scratch/opt/snet

TAG=lpel-base
PCL_VERSION=1.12
SNET_RTS_VERSION=hg-adb51404ec7b
LPEL_VERSION=hg-a0150f29067
SNETC_VERSION=3635
CPPFLAGS_VARIANT =

CPPFLAGS_VARIANT_ = 
CPPFLAGS_VARIANT_w = -DWAITING
CPPFLAGS_VARIANT_cs = -DSCHEDULER_CONCURRENT_PLACEMENT
CPPFLAGS_VARIANT_wcs = -DWAITING -DSCHEDULER_CONCURRENT_PLACEMENT


# where to find the PCL source archive
PCL_ARCHIVE_DIR=/scratch/src
# where to find the lpel sources
LPEL_SOURCES=git://github.com/knz/lpel.git
# where to find the snet sources
SNET_RTS_SOURCES=git://github.com/knz/snet-rts.git
# where to find the snetc sources
SNETC_SOURCES=svn+ssh://svn@svn.snet-home.org/repositories/snet/code/trunk/src/snetc


INSTDIR=$(PREFIX)/p$(PCL_VERSION)_l$(LPEL_VERSION)_s$(SNET_RTS_VERSION)_$(CPPFLAGS_VARIANT)_c$(SNETC_VERSION)
BASESRC=snet-deploy
PCL_SRCDIR=$(BASESRC)/pcl-$(PCL_VERSION)
LPEL_SRCDIR=$(BASESRC)/lpel-$(LPEL_VERSION)
SNET_SRCDIR=$(BASESRC)/snet-$(SNET_RTS_VERSION)-c$(SNETC_VERSION)

$(PREFIX)/$(TAG): $(INSTDIR)/.install-done
	rm -f $@
	printf "# pcl %s\n# lpel %s\n# snet %s\n# snetc %s\n# tag %s\n# dir %s\n" \
	   "$(PCL_VERSION)" "$(LPEL_VERSION)" "$(SNET_RTS_VERSION)" "$(SNETC_VERSION)" "$(TAG)" "$(INSTDIR)" >$@.tmp
	echo "cat <<EOF" >>$@.tmp
	echo "export LD_LIBRARY_PATH=$(INSTDIR)/lib:\$$LD_LIBRARY_PATH ;" >>$@.tmp
	echo "export PATH=$(INSTDIR)/bin:\$$PATH ;" >>$@.tmp
	echo "export SNET_TAG=$(TAG) ;" >>$@.tmp
	echo "export SNET_INCLUDES=$(INSTDIR)/include/snet ;" >>$@.tmp
	echo "export SNET_LIBS=$(INSTDIR)/lib/snet ;" >>$@.tmp
	echo "export SNET_MISC=$(INSTDIR)/share/snet" >>$@.tmp
	echo "EOF" >>$@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@

.PRECIOUS: $(INSTDIR)/.install-done
$(INSTDIR)/.install-done: $(INSTDIR)/.snetc-install-done $(INSTDIR)/clearenv

.PRECIOUS: $(INSTDIR)/clearenv
$(INSTDIR)/clearenv: clearenv
	rm -f $@
	cp -f clearenv $(INSTDIR)/
	chmod +x $@

.PRECIOUS: $(INSTDIR)/.snetc-install-done
$(INSTDIR)/.snetc-install-done: $(SNET_SRCDIR)/.snet-src-done $(INSTDIR)/.snet-install-done
	rm -f $@
	-cd $(SNET_SRCDIR)/src/snetc && $(MAKE) clean
	cd $(SNET_SRCDIR)/src/snetc && \
	   SNET_INCLUDES=$(INSTDIR)/include/snet \
	   SNET_LIBS=$(INSTDIR)/lib/snet \
	   SNET_MISC=$(INSTDIR)/share/snet \
	   ./configure --prefix=$(INSTDIR) CC="$(CC)" CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)" 
	cd $(SNET_SRCDIR)/src/snetc/parse && \
	   $(GREP) -v yylex_destroy scanparse.c >scanparse.c.new && \
	   mv -f scanparse.c.new scanparse.c
	cd $(SNET_SRCDIR)/src/snetc && $(MAKE) MAKEFLAGS=-j1
	$(MKDIR_P) $(INSTDIR)/bin
	cd $(SNET_SRCDIR)/src/snetc && cp -f snetc $(INSTDIR)/bin/ && chmod +x $(INSTDIR)/bin/snetc
	touch $@

.PRECIOUS: $(INSTDIR)/.snet-install-done
$(INSTDIR)/.snet-install-done: $(SNET_SRCDIR)/.snet-src-done $(INSTDIR)/.lpel-install-done
	rm -f $@
	rm -rf $(SNET_SRCDIR)/build
	$(MKDIR_P) $(SNET_SRCDIR)/build
	cd $(SNET_SRCDIR)/build && \
	  ../configure --prefix=$(INSTDIR) \
	   CC="$(CC)" CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS) $(CPPFLAGS_VARIANT_$(CPPFLAGS_VARIANT))" LDFLAGS="$(LDFLAGS)" \
	    --disable-dist-mpi \
	    --with-lpel-includes=$(INSTDIR)/include \
	    --with-lpel-libs=$(INSTDIR)/lib
	cd $(SNET_SRCDIR)/src/runtime/stream/netif && \
	   $(GREP) -v yylex_destroy parser.y >parser.y.new && \
	   mv -f parser.y.new parser.y
	cd $(SNET_SRCDIR)/build && $(MAKE) all V=1
	cd $(SNET_SRCDIR)/build && $(MAKE) install
	touch $@

.PRECIOUS: $(INSTDIR)/.lpel-install-done
$(INSTDIR)/.lpel-install-done: $(LPEL_SRCDIR)/.lpel-src-done $(INSTDIR)/.pcl-install-done
	rm -f $@
	rm -rf $(LPEL_SRCDIR)/build
	$(MKDIR_P) $(LPEL_SRCDIR)/build
	cd $(LPEL_SRCDIR)/build && \
	  ../configure --prefix=$(INSTDIR) \
	   CC="$(CC)" CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS) $(CPPFLAGS_VARIANT_$(CPPFLAGS_VARIANT))" LDFLAGS="$(LDFLAGS)" \
	    --with-pcl=$(INSTDIR)
	cd $(LPEL_SRCDIR)/build && $(MAKE) all V=1
	cd $(LPEL_SRCDIR)/build && $(MAKE) install
	touch $@

.PRECIOUS: $(INSTDIR)/.pcl-install-done
$(INSTDIR)/.pcl-install-done: $(PCL_SRCDIR)/.pcl-src-done
	rm -f $@
	rm -rf $(PCL_SRCDIR)/build
	$(MKDIR_P) $(PCL_SRCDIR)/build
	cd $(PCL_SRCDIR)/build && \
	  ../configure --prefix=$(INSTDIR) CC="$(CC)" CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)"
	cd $(PCL_SRCDIR)/build && $(MAKE) all
	cd $(PCL_SRCDIR)/build && $(MAKE) install
	touch $@

.PRECIOUS: $(PCL_SRCDIR)/.pcl-src-done
$(PCL_SRCDIR)/.pcl-src-done: 
	rm -f $@
	$(MKDIR_P) $(INSTDIR)
	$(MKDIR_P) $(BASESRC)
	rm -rf $(PCL_SRCDIR)
	cd $(BASESRC) && tar -xzf $(PCL_ARCHIVE_DIR)/pcl-$(PCL_VERSION).tar.gz
	test -e $(PCL_SRCDIR)/configure || (echo "PCL archive does not extract to pcl-$(PCL_VERSION)!" >&2; exit 1)
	touch $@

.PRECIOUS: $(LPEL_SRCDIR)/.lpel-src-done
$(LPEL_SRCDIR)/.lpel-src-done:
	rm -f $@
	$(MKDIR_P) $(INSTDIR)
	$(MKDIR_P) $(BASESRC)
	rm -rf $(LPEL_SRCDIR)
	$(GIT) clone $(LPEL_SOURCES) $(LPEL_SRCDIR)
	cd $(LPEL_SRCDIR) && $(GIT) checkout $(LPEL_VERSION)
	cd $(LPEL_SRCDIR) && if test -e bootstrap; then bash bootstrap; else $(AUTORECONF) -v -f -i; fi
	test -e $(PCL_SRCDIR)/configure || (echo "LPEL does not provide configure" >&2; exit 1)
	touch $@

.PRECIOUS: $(SNET_SRCDIR)/.snet-src-done
$(SNET_SRCDIR)/.snet-src-done:
	rm -f $@
	$(MKDIR_P) $(INSTDIR)
	$(MKDIR_P) $(BASESRC)
	rm -rf $(SNET_SRCDIR)
	$(GIT) clone $(SNET_RTS_SOURCES) $(SNET_SRCDIR)
	cd $(SNET_SRCDIR) && $(GIT) checkout $(SNET_RTS_VERSION)
	cd $(SNET_SRCDIR)/src && $(SVN) co -r$(SNETC_VERSION) $(SNETC_SOURCES) snetc
	cd $(SNET_SRCDIR) && if test -e bootstrap; then AUTORECONF="$(AUTORECONF)" bash bootstrap; else $(AUTORECONF) -v -f -i; fi
	cd $(SNET_SRCDIR)/src/snetc && if test -e bootstrap; then AUTORECONF="$(AUTORECONF)" bash bootstrap; else $(AUTORECONF) -v -f -i; fi
	test -e $(SNET_SRCDIR)/configure || (echo "snet-rts does not provide configure" >&2; exit 1)
	test -e $(SNET_SRCDIR)/src/snetc/configure || (echo "snetc does not provide configure" >&2; exit 1)
	touch $@

