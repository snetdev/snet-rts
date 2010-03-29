#
#  Some special rules to built the SDL files properly
#

include $(SACBASE)/stdlib/Makefiles/Makefile.Config

include $(SACBASE)/stdlib/Makefiles/Makefile.standard

#
# add some special flags needed by the SDL sources
#
CCFLAGS := $(CCFLAGS) $(SDLCFLAGS)

#all: lib/libSDLdisplayTree.so

#lib/libSDLdisplayTree.so: SDLdisplay.sac src/$(patsubst %.c,%.o,)

SAC2CFLAGS := $(SAC2CFLAGS) -L lib