
MAINTAINERCLEANFILES = Makefile.in

templatesdir = $(datadir)/$(PACKAGE)/templates
skeletonsdir = $(datadir)/$(PACKAGE)/templates/skeletons
templates_DATA = \
eflproject.edj \
eflproject_python.edj

skeletons_DATA = \
eflproject.tar.gz \
eflproject_python.tar.gz

eflproject.tar.gz:
	tar zcvf eflproject.tar.gz -C skeletons eflproject

eflproject_python.tar.gz:
	tar zcvf eflproject_python.tar.gz -C skeletons eflproject_python

eflproject.edj:
	edje_cc -id images/ eflproject.edc

eflproject_python.edj:
	edje_cc -id images/ eflproject_python.edc

EXTRA_DIST = $(templates_DATA) $(skeletons_DATA)
