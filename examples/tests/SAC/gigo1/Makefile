
# source codes basename
NAME = gigo

# SNET_LIBS and SNET_MISC are environment variables
LIBDIRS   = -Llib -L$(SNET_LIBS)

# snet threading and sac targets should match (though the names may be different)
THREAD    = lpel
SACTARGET = $(THREAD)
LIBVARIANT = -$(SACTARGET)

# exe target name
TARGET = $(NAME)-$(THREAD)

# source config options: LPEL_INCLUDES, LIBLPEL_LA
include $(SNET_MISC)/snet-config.mkf

# sac2c compiles sac sources to binary modules
SAC2C     = sac2c
S2CFLAGS  = -v1 -g -mt -ccflag"$(LPEL_INCLUDES)" -target $(SACTARGET) -nophm

# sac4c generates c wrapper code for external calls of sac modules
SAC4C     = sac4c
S4CFLAGS  = -g -v1 -mt -ccflag"$(LPEL_INCLUDES) $(LIBLPEL_LA)" -target $(SACTARGET) -incdir include -libdir lib -g $(LIBDIRS) -nophm
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(NAME) | sed 's/-pthread//'`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(NAME) | sed 's/-pthread//'`

# sac user type number of the snet handle type
SACTYPE_SNet_SNet = $(shell grep "\#define SACTYPE_SNet_SNet" include/cwrapper.h | cut -f3 -d' ' )
# snet compiler flags
SNETCFLAGS  = -DSACTYPE_SNet_SNet=$(SACTYPE_SNet_SNet) $(S4CINCS) $(S4CLIBS) -threading $(THREAD)

# main target
$(TARGET) : sac-libs $(NAME).snet
	snetc $(SNETCFLAGS) $(NAME).snet -o $(TARGET)

# dependency for the sac module and interface compilation
sac-libs : lib/lib$(NAME)Mod$(LIBVARIANT).so lib/lib$(NAME)Mod$(LIBVARIANT).a lib/lib$(NAME)Tree$(LIBVARIANT).so

# this compile sac sources to modules with sac2c and generates the interface code with sac4c
lib/lib%Mod$(LIBVARIANT).so: %.sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $*.sac -o lib
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $*

clean :
	rm -rf include lib $(NAME).c $(NAME).h *.lo *.o $(TARGET) mon_n*_info.log
