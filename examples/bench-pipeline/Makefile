CC = gcc
CFLAGS = -std=gnu99 -O2 -g -I$(SNET_INCLUDES)
SNETC = snetc

BOXES = boxes/add.c boxes/gen.c boxes/drop.c
BOXLIB = libboxes.a

all:
	@echo "Use: "
	@echo "    make result-L-N-M-P-T-D # run benchmarks; don't use -j to avoid interference" 
	@echo "    make compile            # collate result files into a single output"
	@echo "where:"
	@echo "L = length of pipeline"
	@echo "N = number of records processed"
	@echo "M = amount of work per stage"
	@echo "P = number of cores/workers used (only for lpel)"
	@echo "T = threading back-end (pthread/lpel)"
	@echo "D = distribution back-end (nodist/...)"

clean:
	rm -rf test* 
	rm -f result-* error-*
	rm -f *.hi *.o *\~ 
	rm -f boxes/*.o $(BOXLIB)

$(BOXLIB): $(BOXES:.c=.o)
	$(AR) cru $@ $^
	ranlib $@

.PRECIOUS: test%/prog test%.snet result-% $(BOXLIB)

test%.snet: base-test.snet
	rm -f $@ $@.tmp
	(sed -e "s/NETNAME/test$*/g"< base-test.snet; \
	    printf " gen .. "; \
	    j=0; while test $$j -lt $*; do \
	       printf " add .. "; \
	       j=`expr $$j + 1`; \
	    done; \
	echo " drop;") >$@.tmp
	mv $@.tmp $@

compile: 
	for i in $(wildcard result-*); do \
	  var=`echo $$i | cut -d- -f2- | tr - ' '`; \
	  printf "$$var "; \
	  grep '^real' <$$i | cut -d' ' -f2 | tr '\n' ' '; \
	  echo; \
	done

.SECONDEXPANSION:

WORD1_ = $$(word 1,$$(subst -, ,$$*))
WORD2_ = $$(word 2,$$(subst -, ,$$*))
WORD3_ = $$(word 3,$$(subst -, ,$$*))
WORD4_ = $$(word 4,$$(subst -, ,$$*))
WORD5_ = $$(word 5,$$(subst -, ,$$*))
WORD6_ = $$(word 6,$$(subst -, ,$$*))
WORD7_ = $$(word 7,$$(subst -, ,$$*))

WORD1 = $(word 1,$(subst -, ,$*))
WORD2 = $(word 2,$(subst -, ,$*))
WORD3 = $(word 3,$(subst -, ,$*))
WORD4 = $(word 4,$(subst -, ,$*))
WORD5 = $(word 5,$(subst -, ,$*))
WORD6 = $(word 6,$(subst -, ,$*))
WORD7 = $(word 7,$(subst -, ,$*))

test%/prog: test$(WORD1_).snet $(BOXLIB)
	mkdir -p test$*
	cd test$* && $(SNETC) $(SNETCFLAGS) -L.. -lboxes \
	   -threading $(WORD2) \
	   -distrib   $(WORD3) \
	   -o prog \
	   ../test$(WORD1).snet

input-%.xml: template-input.xml
	rm -f $@ $@.tmp
	sed -e "s/NRECORDS/$(WORD1)/g;s/NWORK/$(WORD2)/g"<template-input.xml >$@.tmp
	mv -f $@.tmp $@

result-%: test$(WORD1_)-$(WORD5_)-$(WORD6_)/prog input-$(WORD2_)-$(WORD3_).xml
	rm -f $@ error-$*
	/usr/bin/time -p ./test$(WORD1)-$(WORD5)-$(WORD6)/prog -w $(WORD4) -i input-$(WORD2)-$(WORD3).xml  >error-$* 2>&1 && \
	 /usr/bin/time -p ./test$(WORD1)-$(WORD5)-$(WORD6)/prog -w $(WORD4) -i input-$(WORD2)-$(WORD3).xml  >>error-$* 2>&1 && \
	 /usr/bin/time -p ./test$(WORD1)-$(WORD5)-$(WORD6)/prog -w $(WORD4) -i input-$(WORD2)-$(WORD3).xml  >>error-$* 2>&1
	mv error-$* $@
