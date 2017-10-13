
MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/skeleton
skeleton_DATA = \
eflproject.tar.gz \
eflproject.edj \
eflproject_python.tar.gz \
eflproject_python.edj

eflproject.tar.gz:
	tar zcvf eflproject.tar.gz -C $(srcdir) eflproject

eflproject_python.tar.gz:
	tar zcvf eflproject_python.tar.gz -C $(srcdir) eflproject_python

eflproject.edj:
	edje_cc -id images/ eflproject.edc

eflproject_python.edj:
	edje_cc -id images/ eflproject_python.edc

EXTRA_DIST = $(skeleton_DATA)
