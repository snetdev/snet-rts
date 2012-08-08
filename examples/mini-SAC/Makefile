LIBDIRS   = -Llib -L$(SNET_LIBS) -L.

SAC2C     = sac2c
S2CFLAGS  = -v1 -g -nophm
SAC4C     = sac4c
S4CFLAGS  =  $(LIBDIRS) -v3 -incdir include -libdir lib -g -o mini
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags mini | sed 's/-pthread//'`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags mini | sed 's/-pthread//'`

TARGETS     = mini
SNETCFLAGS  = -DSACTYPE_SNet_SNet=20 $(S4CINCS) $(S4CLIBS) 
DEPS        = lib/libmini.a


include $(SNET_MISC)/snet.mkf

lib/lib%.a: %.sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $*.sac 
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $*
