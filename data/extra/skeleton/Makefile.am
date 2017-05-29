MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/skeleton
skeleton_DATA = \
eflproject.tar.gz \
eflproject_python.tar.gz 

eflproject.tar.gz:
	tar zcvf eflproject.tar.gz -C $(srcdir) eflproject

eflproject_python.tar.gz:
	tar zcvf eflproject_python.tar.gz -C $(srcdir) eflproject_python

EXTRA_DIST = $(skeleton_DATA)
