#
# $Id$
#


###############################################################################
#
# Makefile for snetc source directories
#
# This Makefile is copied on demand into source directories.
# Be sure not to edit these copies.
#
###############################################################################


###############################################################################
#
# Calling conventions:
#
#  Start rules: 
#    devel       compile developer code (default)
#    prod        compile product code
#    clean       cleanup derived files
#    cleandevel  cleanup compiled files (developer code only)
#    cleanprod   cleanup compiled files (product code only)
#    checkxml    check integrity of XML file
#
#  Parameters:
#    DEPS="no"  de-activate dependency checking meachanism
#    HIDE=""    show important commands issued by make (debugging)
#
###############################################################################


###############################################################################
#
# General settings
#

PROJECT_ROOT = ../../..

include $(PROJECT_ROOT)/src/makefiles/config.mkf
include $(PROJECT_ROOT)/src/makefiles/targets.mkf

TARGETDIR := $(notdir $(CURDIR))

DEPS := yes
HIDE := 


###############################################################################
#
# Dummy rules
#

.PHONY: default devel prod clean cleanprod cleandevel checkxml



###############################################################################
#
# Start rules
#

default: devel

devel: checks
	@$(ECHO) ""
	@$(ECHO) "Making current subdirectory (developer version)"
	@$(TOUCH) .make_track
	$(HIDE) $(MAKE) -C .. -f ../makefiles/core.mkf \
                        TARGETDIR="$(TARGETDIR)" DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        makesubdir.devel
	@$(RM) .make_track

prod: checks
	@$(ECHO) ""
	@$(ECHO) "Making current subdirectory (product version)"
	@$(TOUCH) .make_track
	$(HIDE) $(MAKE) -C .. -f ../makefiles/core.mkf \
                        TARGETDIR="$(TARGETDIR)" DEPS="$(DEPS)" HIDE="$(HIDE)" \
                        makesubdir.prod
	@$(RM) .make_track



###############################################################################
#
# Rules for XML processing
#

checkxml:
	$(XSLTENGINE) check_integrity.xsl ast.xml

%.html: %2html.xsl %.xml
	$(XSLTENGINE) %< ast.xml > $@



###############################################################################
#
# Rules for debugging 
#

TARGETDIRS_DEVEL := $(addprefix ../,$(foreach target,$(src),$(addprefix $(target)/,$($(target)))))


###############################################################################
#
# Rules for directory cleaning
#

clean:
	@$(ECHO) "Cleaning current subdirectory"
	$(HIDE) $(RM) *.o *.a *.bak *~ .*.d
	$(HIDE) $(RM) $(patsubst %.xsl,%,$(wildcard *.xsl))
	$(HIDE) $(RM) -r .sb SunWS_cache
	$(HIDE) $(RM) *.lex.c *.tab.c *.tab.h y.output

cleanprod:
	@$(ECHO) "Cleaning current subdirectory (product version only)"
	$(HIDE) $(RM) *.prod.o

cleandevel:
	@$(ECHO) "Cleaning current subdirectory (developer version only)"
	$(HIDE) $(RM) *.o


###############################################################################
#
# Rules for consistency checks
#

include $(PROJECT_ROOT)/src/makefiles/check.mkf
