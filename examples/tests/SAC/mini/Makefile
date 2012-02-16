LIBDIRS   = -Llib -L$(SNET_LIBS)

SAC2C     = sac2c
S2CFLAGS  = -v1 -g -nophm
SAC4C     = sac4c
S4CFLAGS  = -v3 -incdir include -libdir lib -g $(LIBDIRS)
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags mini | sed 's/-pthread//'`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags mini | sed 's/-pthread//'`

TARGETS     = mini
SNETCFLAGS  = -DSACTYPE_SNet_SNet=21 $(S4CINCS) $(S4CLIBS)
DEPS        = lib/libminiMod.so


include $(SNET_MISC)/snet.mkf

lib/lib%Mod.so: %.sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $*.sac -o lib
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $*
