# S-Net DES demo
# 
# Stephan Herhut (s.a.herhut@herts.ac.uk)
# Frank Penczek (f.penczek@herts.ac.uk)
# 
# $Id: Makefile 45 2008-02-15 15:08:25Z sah $

#
# SUBPROJECTS defines all the subdirectories to make. By default,
# the target all will trigger a make all in the subdirectory,
# whereas the target clean triggers a make clean.
# To override this for a certain project, define projectname_all
# or projectname_clean, respectively.
#
SUBPROJECTS := boxes snet deploy


# 
# nothing to be configured bejond this line
# 

.PHONY: all clean %.all %.clean

all: $(foreach project, $(SUBPROJECTS), $(project).all)

clean: $(foreach project, $(SUBPROJECTS), $(project).clean)

%.all:
	cd $*; $(MAKE) $(if $($*_all),$($*_all),all)

%.clean:
	cd $*; $(MAKE) $(if $($*_clean),$($*_clean),clean)
