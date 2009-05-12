export SHELL=/bin/bash

#---

.SUFFIXES:
.DEFAULT : all

#---

ifneq ($(strip $(NO_SILENT)),true)
.SILENT:
endif

#-------------------------------------------------------------------------------
# Inclusion of options makefiles
#
# Option makefiles specific for the current build are also included (which can 
# ovveride or extend the defaults inclueded above).
include Makefile.opt

#Build specific
-include $(BUILD_SPEC_OPT_DIR)/Makefile.$(SLN_NAME).opt

#---

$(foreach cfg,$(CONFIGS),\
	$(eval -include Makefile.opt.$(cfg)))

#Build specific
$(foreach cfg,$(CONFIGS),\
	$(eval -include $(BUILD_SPEC_OPT_DIR)/Makefile.$(SLN_NAME).opt.$(cfg)))

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# Print function
# 
# NOTE that $1 is arguments to "echo" and $2 is the text
# to be printed! Thus to call it with no arguments to "echo"
# the following must be used (note the double commas).
#
# $(call print,,message)
#

ifneq ($(strip $(NO_PRINT)),true)
define print
@echo $1 "$(subst x, ,$(PRINT_INDENT))$2"
endef
else
define print
endef
endif

#-------------------------------------------------------------------------------

PROJ_NAMES :=

define store_proj_data
PROJ_NAMES += $$(strip $(1))
PROJ_DIR_$$(strip $(1)) := $$(strip $(2))
endef

#---

PROJECTS := $(shell \
    $(SCRIPTS_DIR)/cat-xargs -comment '\#' $(PROJECTS_LST))

#---

$(foreach p,$(PROJECTS),\
	$(eval $(call store_proj_data,\
		$(strip $(shell echo $(p) | sed -e "s/:.*$$//")),\
		$(strip $(shell echo $(p) | sed -e "s/^.*://")))))

#-------------------------------------------------------------------------------

define cfg_prj_tmpl
.PHONY: clean-$(1)_$(2)

ifneq ($$(strip $$(PROJ_CFG_$(2)_$(1))),)
$(1)_$(2): $$(foreach p,$$(PROJ_DEP_$(2)),$(1)_$$(p))
	$$(call print,,"Building $(2) \($(1)\)...")
	
	$$(MAKE) -C           \
		$$(PROJ_DIR_$(2)) \
		$$(PROJ_CFG_$(2)_$(1)) PRINT_INDENT=$$(PRINT_INDENT)xxxx

clean-$(1)_$(2):
	$$(call print,,"Cleaning $(2) \($(1)\)")
	
	$$(MAKE) -C           \
		$$(PROJ_DIR_$(2)) \
		clean-$$(PROJ_CFG_$(2)_$(1)) PRINT_INDENT=$$(PRINT_INDENT)xxxx

else
$(1)_$(2):
clean-$(1)_$(2):

endif
endef

#---

define cfg_tmpl
$(1): $$(foreach p,$$(PROJ_NAMES),$(1)_$$(p))
	$$(call print)
	
clean-$(1): $$(foreach p,$$(PROJ_NAMES),clean-$(1)_$$(p))
	$$(call print)

$$(foreach p,$$(PROJ_NAMES),\
	$$(eval $$(call cfg_prj_tmpl,$(1),$$(p))))

endef

#-------------------------------------------------------------------------------

all: $(CONFIGS) ;

$(foreach p,$(PROJ_NAMES),\
	$(eval all_$(p): $(foreach cfg,$(CONFIGS),$(cfg)_$(p))))

#---

$(foreach p,$(PROJ_NAMES),\
	$(eval clean_$(p): $(foreach cfg,$(CONFIGS),clean-$(cfg)_$(p))))

#---

$(foreach cfg,$(CONFIGS),\
	$(eval $(call cfg_tmpl,$(cfg))))

$(foreach cfg,$(CONFIGS_EXT),\
	$(eval $(call cfg_tmpl,$(cfg))))

#---

clean: $(foreach cfg,$(CONFIGS),clean-$(cfg))
	$(call print)
	rm -f *.log

#---

# Additional rules defined in the options file!
$(eval $(call ADDITIONAL_RULES))

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

