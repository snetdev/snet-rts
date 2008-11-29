.SUFFIXES:
.DEFAULT : all
.IGNORE  : clean 
.PHONY   : update-src-list 

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
#-------------------------------------------------------------------------------

define set_obj_files_var
OBJ_FILES_$(1) := $$(subst .utc,.o,\
	$$(shell $$(SCRIPTS_DIR)/cat-xargs -comment '\#' $$(SRCFILES_LST_$(1))))
 
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

$$(INTERM_DIR_$(1))/%.o: $$(INTERM_DIR_$(1))/%.cpp 
	mkdir -p $$(dir $$@)
	
	$$(CXX)                     \
		$$(CXX_FLAGS)           \
		$$(CXX_FLAGS_$(1))      \
	   	$$(CXX_OUTF_FLAG) $$@   \
		$$(CXX_INCL_DIRS_FLAGS) \
		$$(CXX_INCL_DIRS_FLAGS_$(1)) $$<
endef
	 
#---

define cpp_file_gen_rule_tmpl

$$(INTERM_DIR_$(1))/%.cpp: $$(SOURCE_DIR)/%.utc
	mkdir -p $$(dir $$@)
	
	$$(MAKEDEP)                          \
		$$(MAKEDEP_FLAGS)                \
		$$(MAKEDEP_FLAGS_$(1))           \
		$$(MAKEDEP_TARGET_FLAG) $$@      \
		$$(MAKEDEP_INCL_DIRS_FLAGS)      \
		$$(MAKEDEP_INCL_DIRS_FLAGS_$(1)) \
		$$(MAKEDEP_OUTF_FLAG) $$(@:.cpp=.d) $$<
	
	$$(UTC2CPP)                     \
		$$(UTC2CPP_FLAGS)           \
		$$(UTC2CPP_FLAGS_$(1))      \
		$$(UTC2CPP_OUTF_FLAG) $$@   \
		$$(UTC2CPP_INCL_DIRS_FLAGS) \
		$$(UTC2CPP_INCL_DIRS_FLAGS_$(1)) $$< 
endef

#-------------------------------------------------------------------------------

define build_cfg_tmpl
$$(eval $$(call set_obj_files_var,$(1)))

$$(eval $$(call set_cfg_out_dir_var,INTERM_DIR,$(1)))
$$(eval $$(call set_cfg_out_dir_var,TARGET_DIR,$(1)))

.PRECIOUS: $$(INTERM_DIR_$(1))/%.d
.PRECIOUS: $$(INTERM_DIR_$(1))/%.cpp

$(1): $$(addprefix $$(INTERM_DIR_$(1))/,$$(OBJ_FILES_$(1)))
	mkdir -p $$(TARGET_DIR_$(1))
	
	$$(LD)                                                     \
		$$(LD_FLAGS)                                           \
		$$(LD_FLAGS_$(1))                                      \
		$$(LD_OUTF_FLAG) $$(TARGET_DIR_$(1))/$$(BIN_NAME_$(1)) \
		$$(LD_LIB_DIRS_FLAGS)                                  \
		$$(LD_LIB_DIRS_FLAGS_$(1))                             \
		$$(addprefix $$(INTERM_DIR_$(1))/,$$(OBJ_FILES_$(1)))  \
		$$(LD_LIBS_FLAGS)                                      \
		$$(LD_LIBS_FLAGS_$(1))

$$(eval $$(call cpp_file_gen_rule_tmpl,$(1)))
$$(eval $$(call obj_file_gen_rule_tmpl,$(1)))

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
	
	rm -f  $$(TARGET_DIR_$(1))/$$(BIN_NAME_$(1))
	
	if  [ -d $$(TARGET_DIR_$(1)) ] &&          \
		[ "$$(TARGET_DIR_$(1))" != "." ]; then \
		$$(SCRIPTS_DIR)/rm-dir --ignore-fail-on-non-empty $$(TARGET_DIR_$(1)); fi 

endef

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#Configurations' build rules

all: $(CONFIGS) \
     $(foreach cfg,$(CONFIGS),$(cfg)_so)

#---

$(foreach cfg,$(CONFIGS),\
	$(eval $(call build_cfg_tmpl,$(cfg))))

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#Other rules

$(foreach cfg,$(CONFIGS),\
	$(eval $(call clean_cfg_tmpl,$(cfg))))

#---

clean: $(foreach cfg,$(CONFIGS),clean-$(cfg)) 
	if  [ -d $(INTERM_DIR) ] && \
		[ "$(INTERM_DIR)" != "." ]; then rm -rf $(INTERM_DIR); fi
	
	rm -f  $(SRCFILES_LST)~
	
	if  [ -d $(TARGET_DIR) ] &&          \
		[ "$(TARGET_DIR)" != "." ]; then \
		$(SCRIPTS_DIR)/rm-dir --ignore-fail-on-non-empty $(TARGET_DIR); fi

#---

update-src-list:
	$(SCRIPTS_DIR)/autoupdate-srclst \
		-b -ext utc -ex-ext exclude -o $(SRCFILES_LST) $(SOURCE_DIR)/

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

