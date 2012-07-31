.PRECIOUS: testpf%.snet
testpf%.snet: base-test.snet patterns.mk
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)(sed -e "s/NETNAME/testpf$*/g"< base-test.snet; \
	    printf " gen .. "; \
	    j=0; while test $$j -lt $*; do \
	       printf " add .. "; \
	       j=`expr $$j + 1`; \
	    done; \
	  echo " drop;") >$@.tmp
	$(V_at)mv $@.tmp $@

.PRECIOUS: testpt%.snet
testpt%.snet: base-test.snet patterns.mk
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)(sed -e "s/NETNAME/testpt$*/g"< base-test.snet; \
	    printf " f2t .. gent .. "; \
	    j=0; while test $$j -lt $*; do \
	       printf " addt .. "; \
	       j=`expr $$j + 1`; \
	    done; \
	  echo " dropt;") >$@.tmp
	$(V_at)mv $@.tmp $@

.PRECIOUS: testb%.snet
testb%.snet: base-test.snet patterns.mk
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)(sed -e "s/NETNAME/testb$*/g"< base-test.snet; \
	    printf " f2t .. gent .. "; \
	    j=0; while test $$j -lt $*; do \
	       printf " (addt!<c>) .. "; \
	       j=`expr $$j + 1`; \
	    done; \
	 echo " dropt;") >$@.tmp
	$(V_at)mv $@.tmp $@

.PRECIOUS: tests%.snet
tests%.snet: base-test.snet patterns.mk
	$(V_at)rm -f $@ $@.tmp
	$(V_GEN)(sed -e "s/NETNAME/tests$*/g"< base-test.snet; \
	    printf " f2t .. gent .. "; \
	    j=0; while test $$j -lt $*; do \
	       printf " (addt .. [{<c>} -> if <c==1> then {<c=c>,<stop>} else {<c=c-1>}] ) *{<stop>} .. "; \
	       j=`expr $$j + 1`; \
	    done; \
	 echo " dropt;") >$@.tmp
	$(V_at)mv $@.tmp $@

