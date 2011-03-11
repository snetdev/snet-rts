
#
# $Id$
#


###############################################################################
#
# Calling conventions:
#
#  Start rules:
#    default    compile executables as developer code and libraries as product
#               code
#    devel      compile everything as developer code
#    prod       compile everything as product code
#    clean      cleanup all derived files
#    cleandevel cleanup only compiled files (developer code)
#    cleanprod  cleanup only compiled files (product code)
#
#  Parameters:
#    DEPS="no"  de-activate dependency checking mechanism
#    HIDE=""    show important commands issued by make (debugging)
#
###############################################################################


###############################################################################
#
# general setup:
#

PROJECT_ROOT := .

HIDE := @
DEPS := yes

include $(PROJECT_ROOT)/src/makefiles/config.mkf


###############################################################################
#
# Dummy rules
#

.PHONY: default devel prod clean cleandevel cleanprod efence config


###############################################################################
#
# Start rules
#
# The definition of these rules deliberately enforces a sequence in compilation
# rather than expressing the dependencies properly by makefile rules.
#
# The rationale is that commonlib and maketools rather seldomly require
# recompilation. With proper dependencies, however, they would require a
# dependency from every compilation rule. This would lead to extensive
# rechecking at compile time that is absolutely superfluous.
#
# The runtime system may require the compiler and, likewise, the tools may
# need the runtime system or parts thereof. Expressing all this by dependencies
# would lead to a system in which the tools form the main target. This seems
# unnatural.
#
# Furthermore, the current solution allows us to rebuild locally without 
# enforcing dependency checks.
#

default devel prod: checks
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Building S-Net"
	@$(ECHO) "************************************************************"
	$(HIDE) mkdir -p lib src/bin src/lib
	$(HIDE) $(MAKE) -C src/commonlib DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/commonlib/" PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/maketools DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/maketools/" PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/util   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/util/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/compiler  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/compiler/"  PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/lpellib   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/lpellib/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/runtime_lpel  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/runtime_lpel/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/distrib   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/distrib/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/tools     DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/tools/"     PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/demo     DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/demo/"     PREFIX_ROOT="" $@ 
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Building S-Net completed"
	@$(ECHO) "************************************************************"
	@$(ECHO) ""

clean cleandevel cleanprod:
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Cleaning S-Net"
	@$(ECHO) "************************************************************"
	$(HIDE) $(MAKE) -C src/commonlib DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/commonlib/" PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/maketools DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/maketools/" PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/util   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/util/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/compiler  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/compiler/"  PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/lpellib   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/lpellib/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/runtime_lpel   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/runtime_lpel/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/distrib   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/distrib/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/tools     DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/tools/"     PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/demo     DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/demo/"     PREFIX_ROOT="" $@
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Cleaning S-Net completed"
	@$(ECHO) "************************************************************"
	@$(ECHO) ""

efence:
	$(HIDE) $(MAKE) -C src/compiler  DEPS="$(DEPS)" HIDE="$(HIDE)" $@


###############################################################################
#
# Rules for configure mechanism
#

config:
	svn lock configure $(AUTOHEADERED)
	autoconf
	autoheader
	svn commit configure $(AUTOHEADERED)


###############################################################################
#
# Rules for consistency checks
#

include $(PROJECT_ROOT)/src/makefiles/check.mkf
