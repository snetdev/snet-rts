
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
# rather than expressing the dependencies properly by makefile rules, because
# the author of this Makefile finds the dependency tracking difficult to
# implement.

default devel prod: checks
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Building S-Net"
	@$(ECHO) "************************************************************"
	$(HIDE) mkdir -p lib src/bin src/lib
	$(HIDE) $(MAKE) -C src/snetc  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/snetc/"  PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/util   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/util/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/runtime/ DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/runtime/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/runtime/stream  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/runtime/stream/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/distrib   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/distrib/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/threading   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/threading/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/interfaces/c4snet HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/interfaces/c4snet/" PREFIX_ROOT="" $@
	#$(HIDE) $(MAKE) -C src/interfaces/sac4snet HIDE="$(HIDE)" \
        #               PREFIX_LOCAL="src/interfaces/sac4snet/" PREFIX_ROOT="" $@
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
	$(HIDE) $(MAKE) -C src/snetc  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/snetc/"  PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/util   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/util/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/runtime/ DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/runtime/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/runtime/stream  DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/runtime/stream/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/distrib   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/distrib/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/threading   DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/threading/"   PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/interfaces/c4snet HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/interfaces/c4snet/" PREFIX_ROOT="" $@
	$(HIDE) $(MAKE) -C src/interfaces/sac4snet HIDE="$(HIDE)" \
                        PREFIX_LOCAL="src/interfaces/sac4snet/" PREFIX_ROOT="" $@
	@$(ECHO) ""
	@$(ECHO) "************************************************************"
	@$(ECHO) "* Cleaning S-Net completed"
	@$(ECHO) "************************************************************"
	@$(ECHO) ""

efence:
	$(HIDE) $(MAKE) -C src/snetc  DEPS="$(DEPS)" HIDE="$(HIDE)" $@


###############################################################################
#
# Rules for configure mechanism
#

config:
	autoreconf -v -f -i -I config


###############################################################################
#
# Rules for consistency checks
#

include $(PROJECT_ROOT)/src/makefiles/check.mkf
