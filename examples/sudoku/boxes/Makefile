
# - - - - - - - - - - - - - - - - - - - -
SACTARGET = sudoku
# - - - - - - - - - - - - - - - - - - - -
LIBDIRS   = -L$(SNETBASE)/lib

SAC2C     = sac2c
S2CFLAGS  = -v1 -g
SAC4C     = sac4c
S4CFLAGS  = -v3 -incdir include -libdir lib -g $(LIBDIRS) -o boxes

.PHONY: all clean

all: include/boxes.h

include/boxes.h: lib$(SACTARGET)Mod.so
	mkdir -p lib include
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $(SACTARGET)

lib$(SACTARGET)Mod.so: src/$(SACTARGET).sac
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $<

clean:
	rm -fr lib/ include/
	rm -f *.so *.a
