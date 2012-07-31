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

WORD1 = $(word 1,$(subst -, ,$*))
WORD2 = $(word 2,$(subst -, ,$*))
WORD3 = $(word 3,$(subst -, ,$*))
WORD4 = $(word 4,$(subst -, ,$*))
WORD5 = $(word 5,$(subst -, ,$*))
WORD6 = $(word 6,$(subst -, ,$*))
WORD7 = $(word 7,$(subst -, ,$*))
WORD8 = $(word 8,$(subst -, ,$*))

test%/prog: test$(WORD1_)$(WORD2_).snet $(BOXLIB)
	$(V_at)mkdir -p test$*
	$(V_at)cd test$* && rm -f compile.log
	$(V_GEN)cd test$* && ($(SNETC) $(SNETCFLAGS) -L.. -lboxes \
	   -threading $(WORD3) \
	   -distrib   $(WORD4) \
	   -o prog \
	   ../test$(WORD1)$(WORD2).snet >compile.log 2>&1 || { r=$$?; cat compile.log; exit $$r ;})
	$(V_at)if $(V_P); then cat test$*/compile.log; fi

input-%.xml: template-input.xml
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)sed -e "s/NRECORDS/$(WORD1)/g;s/NWORK/$(WORD2)/g;s/CYCLIC/$(WORD3)/g"<template-input.xml >$@.tmp
	$(V_at)mv -f $@.tmp $@

result-%: test-$(WORD1_)-$(WORD2_)-$(WORD7_)-$(WORD8_)/prog input-$(WORD3_)-$(WORD4_)-$(WORD5_).xml
	$(V_at)rm -f $@ error-$*
	$(V_GEN)(/usr/bin/time -p \
	   ./test-$(WORD1)-$(WORD2)-$(WORD7)-$(WORD8)/prog -w $(WORD6) -i input-$(WORD3)-$(WORD4)-$(WORD5).xml >error-$* 2>&1 && \
	/usr/bin/time -p \
	   ./test-$(WORD1)-$(WORD2)-$(WORD7)-$(WORD8)/prog -w $(WORD6) -i input-$(WORD3)-$(WORD4)-$(WORD5).xml  >>error-$* 2>&1 && \
	 /usr/bin/time -p \
	   ./test-$(WORD1)-$(WORD2)-$(WORD7)-$(WORD8)/prog -w $(WORD6) -i input-$(WORD3)-$(WORD4)-$(WORD5).xml  >>error-$* 2>&1) \
	  || { r=$$?; \
	       echo "  FAIL     ./test-$(WORD1)-$(WORD2)-$(WORD7)-$(WORD8)/prog -w $(WORD6) -i input-$(WORD3)-$(WORD4)-$(WORD5).xml" >&2; \
	       echo "  ERROR    -> error-$*" >&2; \
	       if $(V_P); then cat error-$*; fi; \
	       exit $$r; }
	$(V_at)mv error-$* $@
