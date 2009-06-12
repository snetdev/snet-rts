.SUFFIXES:
.DEFAULT : all
.IGNORE  : clean 
.PHONY   : update-src-list 

#---

ifneq ($(strip $(NO_SILENT)),true)
.SILENT:
endif

#---

ifeq ($(MAKELEVEL),0)
export SHELL                  := /bin/bash
export TOP_LEVEL_MAKEFILE_DIR := $(shell pwd)
endif

#-------------------------------------------------------------------------------
# Inclusion of options makefiles
#
# Option makefiles specific for the current build are also included (which can 
# ovveride or extend the defaults inclueded above).
include Makefile.opt

#Build specific
-include $(BUILD_SPEC_OPT_DIR)/Makefile.$(PROJ_NAME).opt

#---

$(foreach cfg,$(CONFIGS),\
	$(eval -include Makefile.opt.$(cfg)))

#Build specific
$(foreach cfg,$(CONFIGS),\
	$(eval -include $(BUILD_SPEC_OPT_DIR)/Makefile.$(PROJ_NAME).opt.$(cfg)))

#-------------------------------------------------------------------------------
# Default values for output / intermediate  directories

ifeq ($(strip $(INTERM_DIR)),)
	INTERM_DIR := intermediate
endif

ifeq ($(strip $(TARGET_DIR)),)
	TARGET_DIR := .
endif

#-------------------------------------------------------------------------------
# Script that deals with stupid compilers and other tools that
# insist on printing crap other than errors / warnings to stdout or stderr.
#
EXEC_SILENT := $(SCRIPTS_DIR)/exec-silent

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
endif

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

define set_files_vars
OBJ_FILES_$(1) := $$(subst .utc,.o,\
	$$(shell $$(SCRIPTS_DIR)/cat-xargs -comment '\#' $$(SRC_FILES_LST_$(1))))
 
OBJ_FILES_$(1) := $$(subst $$(SOURCE_DIR),,$$(OBJ_FILES_$(1)))
OBJ_FILES_$(1) := $$(patsubst /%,%,$$(OBJ_FILES_$(1)))
endef

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# Functions for setting up the output / intermediate directories.

define set_cfg_out_dir_var
ifeq ($$(strip $$($(1)_$(2))),)
	$(1)_$(2) := $$($(1))/$(2)
else
	$(1)_$(2) := $$($(1))/$$($(1)_$(2))
endif
endef

#-------------------------------------------------------------------------------

define obj_file_gen_rule_tmpl

$$(INTERM_DIR_$(1))/%.o : $$(INTERM_DIR_$(1))/%.cpp
	$$(call print,,"Compiling: $$<")
	
	mkdir -p $$(dir $$@)
	
	$$(CXX$(2))                     \
		$$(CXX_FLAGS$(2))           \
		$$(CXX_FLAGS_$(1))          \
	   	$$(CXX_OUTF_FLAG$(2)) $$@   \
		$$(CXX_INCL_DIRS_FLAGS$(2)) \
		$$(CXX_INCL_DIRS_FLAGS_$(1)) $$<

endef
	 
#---

define cpp_file_gen_rule_tmpl

$$(INTERM_DIR_$(1))/%.cpp: $$(SOURCE_DIR)/%.utc
	$$(call print,,"Translating: $$<")
	
	mkdir -p $$(dir $$@)
	
	$$(MAKEDEP$(2))                      \
		$$(MAKEDEP_FLAGS$(2))            \
		$$(MAKEDEP_FLAGS_$(1))           \
		$$(MAKEDEP_TARGET_FLAG$(2)) $$@  \
		$$(MAKEDEP_INCL_DIRS_FLAGS$(2))  \
		$$(MAKEDEP_INCL_DIRS_FLAGS_$(1)) \
		$$(MAKEDEP_OUTF_FLAG$(2)) $$(@:.cpp=.d) $$<
	
	$$(UTC2CPP$(2))                     \
		$$(UTC2CPP_FLAGS$(2))           \
		$$(UTC2CPP_FLAGS_$(1))          \
		$$(UTC2CPP_OUTF_FLAG$(2)) $$@   \
		$$(UTC2CPP_INCL_DIRS_FLAGS$(2)) \
		$$(UTC2CPP_INCL_DIRS_FLAGS_$(1)) $$< 
endef

#-------------------------------------------------------------------------------

define build_cfg_tmpl
$$(eval $$(call set_files_vars,$(1)))

$$(eval $$(call set_cfg_out_dir_var,INTERM_DIR,$(1)))
$$(eval $$(call set_cfg_out_dir_var,TARGET_DIR,$(1)))

.PRECIOUS: $$(INTERM_DIR_$(1))/%.d
.PRECIOUS: $$(INTERM_DIR_$(1))/%.cpp

.PHONY:    $$(PREREQUISITES_$(1))

$(1): $$(PREREQUISITES_$(1)) \
      $$(TARGET_DIR_$(1))/$$(BIN_NAME_$(1))

$$(TARGET_DIR_$(1))/$$(BIN_NAME_$(1)): \
      $$(addprefix $$(INTERM_DIR_$(1))/,$$(OBJ_FILES_$(1)))
	
	$$(call print,,"Linking: $$(BIN_NAME_$(1))")
	
	mkdir -p $$(TARGET_DIR_$(1))
	
	$$(LD$(2))                                                     \
		$$(LD_FLAGS$(2))                                           \
		$$(LD_FLAGS_$(1))                                          \
		$$(LD_OUTF_FLAG$(2)) $$(TARGET_DIR_$(1))/$$(BIN_NAME_$(1)) \
		$$(LD_LIB_DIRS_FLAGS$(2))                                  \
		$$(LD_LIB_DIRS_FLAGS_$(1))                                 \
		$$(addprefix $$(INTERM_DIR_$(1))/,$$(OBJ_FILES_$(1)))      \
		$$(LD_LIBS_FLAGS$(2))                                      \
		$$(LD_LIBS_FLAGS_$(1))

$$(eval $$(call cpp_file_gen_rule_tmpl,$(1),$(2)))
$$(eval $$(call obj_file_gen_rule_tmpl,$(1),$(2)))

-include $$(wildcard $$(INTERM_DIR_$(1))/*.d)
endef

#-------------------------------------------------------------------------------

define clean_cfg_tmpl
.PHONY : clean-$(1)
.IGNORE: clean-$(1)

clean-$(1):
	if  [ -d $$(INTERM_DIR_$(1)) ] && \
		[ "$$(INTERM_DIR_$(1))" != "." ]; then rm -rf $$(INTERM_DIR_$(1)); fi
	
	if  [ -d $$(INTERM_DIR) ] &&          \
		[ "$$(INTERM_DIR)" != "." ]; then \
		$$(SCRIPTS_DIR)/rm-dir --ignore-fail-on-non-empty $$(INTERM_DIR); fi
	
	rm -f  $$(TARGET_DIR_$(1))/$$(BIN_NAME_$(1))*
	
	if  [ -d $$(TARGET_DIR_$(1)) ] &&          \
		[ "$$(TARGET_DIR_$(1))" != "." ]; then \
		$$(SCRIPTS_DIR)/rm-dir --ignore-fail-on-non-empty $$(TARGET_DIR_$(1)); fi 

endef

#-------------------------------------------------------------------------------

define make_config_build_toolset_var
$(if $(BUILD_TOOLSET_$(1)),_$(BUILD_TOOLSET_$(1)),)
endef

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#Configurations' build rules

all: $(CONFIGS) ;

#---

$(foreach cfg,$(CONFIGS),\
	$(eval $(call build_cfg_tmpl,$(cfg),$(call \
		make_config_build_toolset_var,$(cfg)))))

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#Other rules

$(foreach cfg,$(CONFIGS),\
	$(eval $(call clean_cfg_tmpl,$(cfg))))

#---

clean: $(foreach cfg,$(CONFIGS),clean-$(cfg)) 
	if  [ -d $(INTERM_DIR) ] && \
		[ "$(INTERM_DIR)" != "." ]; then rm -rf $(INTERM_DIR); fi
	
	if  [ -d $(TARGET_DIR) ] &&          \
		[ "$(TARGET_DIR)" != "." ]; then \
		$(SCRIPTS_DIR)/rm-dir --ignore-fail-on-non-empty $(TARGET_DIR); fi

#---

update-src-list:
	$(SCRIPTS_DIR)/autoupdate-srclst \
		-ext utc -ex-ext exclude -o $(SRC_FILES_LST) $(SOURCE_DIR)/

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

