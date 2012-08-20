PYTHON = python
CC = gcc
CFLAGS = -std=gnu99 -O2 -g -I$(SNET_INCLUDES)
SNETC = snetc
TAG = 

all:
	@echo "Use: "
	@echo "    make check                    # sanity check that everything works fine"
	@echo "    make result-B-L-W-N-M-C-P-T-D # run benchmarks; don't use -j to avoid interference" 
	@echo "    make compile                  # collate result files into a single output"
	@echo "where:"
	@echo "B = type of inner network"
	@echo "    pt = pipeline of tag-based boxes"
	@echo "    bn = pipeline of blings"
	@echo "    bd = pipeline of deterministic blings"
	@echo "    sn = pipeline of stars"
	@echo "    sd = pipeline of deterministic stars"
	@echo "L = length of pipeline"
	@echo "W = width of parallel"
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
	$(V_at)echo "# B L W N M C P T D timings..." 
	$(V_at)ls result-* | while read i; do \
	  var=`echo $$i | cut -d- -f2- | tr - ' '`; \
	  printf "$$var "; \
	  vals=`grep '^real' <$$i | grep -v '0.00' | cut -c5- | tr -d ' \t' | tr '\n' ','`; \
	  if test "x$$vals" = x; then echo "- -"; else \
	  $(PYTHON) -c "print min([$$vals]), ' '.join((str(x) for x in [$$vals]))"; \
	  fi; \
	done

##### Things that get tested with "make check" #####

T_BASES = sn sd pt bn bd
T_LENS = 1 50
T_WIDTHS = 0 1 5
T_NRECS = 1 50
T_CYCLES = 1 50
T_THREADING = lpel pthread
T_CORES = 1 2
T_LOADS = 1

TESTS = $(foreach B,$(T_BASES), \
	$(foreach L,$(T_LENS), \
	$(foreach W,$(T_WIDTHS), \
	$(foreach N,$(T_NRECS), \
	$(foreach M,$(T_LOADS), \
	$(foreach C,$(T_CYCLES), \
	$(foreach P,$(T_CORES), \
	$(foreach T,$(T_THREADING), \
	$(B)-$(L)-$(W)-$(N)-$(M)-$(C)-$(P)-$(T)-nodist))))))))

check:
	$(MAKE) $(TESTS:%=result-%$(TAG))

##### Automated testing/benchmarking #####

include boxes.mk

# test generator
.PRECIOUS: test_%.snet
test_%.snet: base-test.snet patterns.mk genpat.py
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)(sed -e "s/NETNAME/test_$*/g"< base-test.snet && $(PYTHON) genpat.py $* ) >$@.tmp
	$(V_at)mv $@.tmp $@

include run.mk

