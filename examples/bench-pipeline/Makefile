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

test%/prog: $$(word 1,$$(subst -, ,$$@)).snet $(BOXLIB)
	mkdir -p test$*
	cd test$* && $(SNETC) $(SNETCFLAGS) -L.. -lboxes \
	   -threading $(word 2,$(subst -, ,$*)) \
	   -distrib   $(word 3,$(subst -, ,$*)) \
	   -o prog \
	   ../test$(word 1,$(subst -, ,$*)).snet

result-%: test$$(word 2,$$(subst -, ,$$@))-$$(word 6,$$(subst -, ,$$@))-$$(word 7,$$(subst -, ,$$@))/prog template-input.xml
	rm -f $@ error-$*
	len=`echo $*|cut -d- -f1`; \
	nrecs=`echo $*|cut -d- -f2`; \
	nwork=`echo $*|cut -d- -f3`; \
	ncores=`echo $*|cut -d- -f4`; \
	impl=`echo $*|cut -d- -f5-6`; \
	sed -e "s/NRECORDS/$$nrecs/g;s/NWORK/$$nwork/g"<template-input.xml | \
	  /usr/bin/time -p ./test$$len-$$impl/prog -w $$ncores  >error-$* 2>&1 && \
	sed -e "s/NRECORDS/$$nrecs/g;s/NWORK/$$nwork/g"<template-input.xml | \
	  /usr/bin/time -p ./test$$len-$$impl/prog -w $$ncores  >>error-$* 2>&1 && \
	sed -e "s/NRECORDS/$$nrecs/g;s/NWORK/$$nwork/g"<template-input.xml | \
	  /usr/bin/time -p ./test$$len-$$impl/prog -w $$ncores  >>error-$* 2>&1
	mv error-$* $@
