#
#  Some special rules to built the SDL files properly
#

include $(SACBASE)/stdlib/Makefiles/Makefile.Config

ifneq ($(ENABLE_SDL),yes)
  EXCLUDE_FILES := drawArray.c drawPixel.c initDisplay.c destroyDisplay.c \
		   drawArrayOffset.c $(EXCLUDE_FILES) 
endif

include $(SACBASE)/stdlib/Makefiles/Makefile_template.stdlib

#
# add some special flags needed by the SDL sources
#
CCFLAGS := $(CCFLAGS) $(SDLCFLAGS) -g

