MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/skeleton
skeleton_DATA = eflproject.tar.gz

eflproject.tar.gz:
	tar zcvf eflproject.tar.gz -C $(srcdir) eflproject

EXTRA_DIST = $(skeleton_DATA)
