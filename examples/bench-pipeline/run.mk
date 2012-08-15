V_GEN = $(v_GEN_$(V))
v_GEN_ = $(v_GEN_0)
v_GEN_0 = @echo "  GEN     " $@;
v_GEN_1 = 
V_at = $(v_at_$(V))
v_at_ = $(v_at_0)
v_at_0 = @
v_at_1 = 
V_P = $(v_P_$(V))
v_P_ = $(v_P_0)
v_P_0 = false
v_P_1 = :

.PRECIOUS: test%/prog result-% input-%.xml

.SECONDEXPANSION:

WORD1_ = $$(word 1,$$(subst -, ,$$*))
WORD2_ = $$(word 2,$$(subst -, ,$$*))
WORD3_ = $$(word 3,$$(subst -, ,$$*))
WORD4_ = $$(word 4,$$(subst -, ,$$*))
WORD5_ = $$(word 5,$$(subst -, ,$$*))
WORD6_ = $$(word 6,$$(subst -, ,$$*))
WORD7_ = $$(word 7,$$(subst -, ,$$*))
WORD8_ = $$(word 8,$$(subst -, ,$$*))
WORD9_ = $$(word 9,$$(subst -, ,$$*))

WORD1 = $(word 1,$(subst -, ,$*))
WORD2 = $(word 2,$(subst -, ,$*))
WORD3 = $(word 3,$(subst -, ,$*))
WORD4 = $(word 4,$(subst -, ,$*))
WORD5 = $(word 5,$(subst -, ,$*))
WORD6 = $(word 6,$(subst -, ,$*))
WORD7 = $(word 7,$(subst -, ,$*))
WORD8 = $(word 8,$(subst -, ,$*))
WORD9 = $(word 9,$(subst -, ,$*))

test%/prog: test_$(WORD1_)_$(WORD2_)_$(WORD3_).snet $(BOXLIB)
	$(V_at)mkdir -p test$*
	$(V_at)cd test$* && rm -f compile.log
	$(V_GEN)cd test$* && ($(SNETC) $(SNETCFLAGS) -L.. -lboxes \
	   -threading $(WORD4) \
	   -distrib   $(WORD5) \
	   -o prog \
	   ../test_$(WORD1)_$(WORD2)_$(WORD3).snet >compile.log 2>&1 || { r=$$?; cat compile.log; exit $$r ;})
	$(V_at)if $(V_P); then cat test$*/compile.log; fi

input-%.xml: template-input.xml
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)sed -e "s/NRECORDS/$(WORD1)/g;s/NWORK/$(WORD2)/g;s/CYCLIC/$(WORD3)/g"<template-input.xml >$@.tmp
	$(V_at)mv -f $@.tmp $@

result-%: test-$(WORD1_)-$(WORD2_)-$(WORD3_)-$(WORD8_)-$(WORD9_)/prog input-$(WORD4_)-$(WORD5_)-$(WORD6_).xml
	$(V_at)rm -f $@ error-$*
	$(V_GEN)( touch error-$*; \
	    i=0; while test $$i -lt $${BENCH_RUNS:-3}; do \
            if ! /usr/bin/time -p \
	      ./test-$(WORD1)-$(WORD2)-$(WORD3)-$(WORD8)-$(WORD9)/prog -w $(WORD7) \
	          -i input-$(WORD4)-$(WORD5)-$(WORD6).xml \
	          $${BENCH_EXTRA_ARGS:-} \
                  >>error-$* 2>&1; then \
                  exit 1; \
	    fi; \
	    i=`expr $$i + 1`; \
	    done ) || { r=$$?; \
	       echo "  FAIL     ./test-$(WORD1)-$(WORD2)-$(WORD3)-$(WORD8)-$(WORD9)/prog -w $(WORD7) -i input-$(WORD4)-$(WORD5)-$(WORD6).xml $${BENCH_EXTRA_ARGS:-}" >&2; \
	       echo "  ERROR    -> error-$*" >&2; \
	       if $(V_P); then cat error-$*; fi; \
	       exit $$r; }
	$(V_at)mv error-$* $@
