
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
#    DEPS="no"  de-activate dependency checking meachanism 
#    HIDE=""    show important commands issued by make (debugging)
#
###############################################################################


#######################################################################################
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

.PHONY: default devel prod clean cleandevel cleanprod efence


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

default devel prod: 
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Building S-Net"
	@$(ECHO) "************************************************************"
	$(HIDE) $(MAKE) -C src/commonlib DEPS="$(DEPS)" HIDE="$(HIDE)" $@
	$(HIDE) $(MAKE) -C src/maketools DEPS="$(DEPS)" HIDE="$(HIDE)" $@
	$(HIDE) $(MAKE) -C src/compiler  DEPS="$(DEPS)" HIDE="$(HIDE)" $@
#	$(HIDE) $(MAKE) -C src/runtime   DEPS="$(DEPS)" HIDE="$(HIDE)" $@
#	$(HIDE) $(MAKE) -C src/tools     DEPS="$(DEPS)" HIDE="$(HIDE)" $@
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
	$(HIDE) $(MAKE) -C src/commonlib DEPS="$(DEPS)" HIDE="$(HIDE)" $@
	$(HIDE) $(MAKE) -C src/maketools DEPS="$(DEPS)" HIDE="$(HIDE)" $@
	$(HIDE) $(MAKE) -C src/compiler  DEPS="$(DEPS)" HIDE="$(HIDE)" $@
#	$(HIDE) $(MAKE) -C src/runtime   DEPS="$(DEPS)" HIDE="$(HIDE)" $@
#	$(HIDE) $(MAKE) -C src/tools     DEPS="$(DEPS)" HIDE="$(HIDE)" $@
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
# These rules are mainly here as a reminder what exactly to do when 
# reconfiguring.
#

$(PROJECT_ROOT)/src/makefiles/config.mkf: $(PROJECT_ROOT)/src/makefiles/config.mkf.in
	(cd $(PROJECT_ROOT); ./configure)

configure: configure.ac
	svn lock configure src/global/config.h.in
	autoconf
	autoheader
	svn commit -m "Reconfigured" configure src/global/config.h.in



