CC = gcc
CFLAGS = -std=gnu99 -O2 -g -I$(SNET_INCLUDES)
SNETC = snetc

all:
	@echo "Use: "
	@echo "    make check                  # sanity check that everything works fine"
	@echo "    make result-B-L-N-M-C-P-T-D # run benchmarks; don't use -j to avoid interference" 
	@echo "    make compile                # collate result files into a single output"
	@echo "where:"
	@echo "B = type of inner network"
	@echo "    pf = pipeline of field-based boxes"
	@echo "    pt = pipeline of tag-based boxes"
	@echo "L = length of pipeline"
	@echo "N = number of records processed"
	@echo "M = amount of work per stage"
	@echo "C = max amount of inner repetition"
	@echo "P = number of cores/workers used (only for lpel)"
	@echo "T = threading back-end (pthread/lpel)"
	@echo "D = distribution back-end (nodist/...)"

##### Common rules #####

clean: clean-boxes
	rm -rf test* 
	rm -f result-* error-* input-*

compile: 
	for i in $(wildcard result-*); do \
	  var=`echo $$i | cut -d- -f2- | tr - ' '`; \
	  printf "$$var "; \
	  grep '^real' <$$i | cut -d' ' -f2 | tr '\n' ' '; \
	  echo; \
	done

##### Things that get tested with "make check" #####

T_BASES = s pf pt b
T_LENS = 1 10 100
T_NRECS = 1 10 100
T_CYCLES = 1 10 100
T_THREADING = lpel pthread
T_CORES = 1 2

TESTS = $(foreach B,$(T_BASES), \
	$(foreach L,$(T_LENS), \
	$(foreach N,$(T_NRECS), \
	$(foreach C,$(T_CYCLES), \
	$(foreach P,$(T_CORES), \
	$(foreach T,$(T_THREADING), \
	$(B)-$(L)-$(N)-1-$(C)-$(P)-$(T)-nodist))))))

check:
	rm -f $(TESTS:%=result-%) $(TESTS:%=error-%)
	$(MAKE) $(TESTS:%=result-%)

##### Automated testing/benchmarking #####

include boxes.mk
include patterns.mk
include run.mk

