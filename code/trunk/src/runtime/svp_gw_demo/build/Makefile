.SUFFIXES:
.DEFAULT : all

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

$(1)_$(2): $$(foreach p,$$(PROJ_DEP_$(2)),$(1)_$$(p))
	$$(MAKE) -C $$(PROJ_DIR_$(2)) $$(PROJ_CFG_$(2)_$(1))

clean-$(1)_$(2):
	$$(MAKE) -C $$(PROJ_DIR_$(2)) clean-$$(PROJ_CFG_$(2)_$(1))

endef

#---

define cfg_tmpl
$(1)      : $$(foreach p,$$(PROJ_NAMES),$(1)_$$(p))
clean-$(1): $$(foreach p,$$(PROJ_NAMES),clean-$(1)_$$(p))

$$(foreach p,$$(PROJ_NAMES),\
	$$(eval $$(call cfg_prj_tmpl,$(1),$$(p))))

endef

#-------------------------------------------------------------------------------

all: $(foreach cfg,$(CONFIGS),$(cfg))

$(foreach p,$(PROJ_NAMES),\
	$(eval all_$(p): $(foreach cfg,$(CONFIGS),$(cfg)_$(p))))

#---

$(foreach p,$(PROJ_NAMES),\
	$(eval clean_$(p): $(foreach cfg,$(CONFIGS),clean-$(cfg)_$(p))))

#---

$(foreach cfg,$(CONFIGS),\
	$(eval $(call cfg_tmpl,$(cfg))))

#---

clean: $(foreach cfg,$(CONFIGS),clean-$(cfg))

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

