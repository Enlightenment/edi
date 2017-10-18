
MAINTAINERCLEANFILES = Makefile.in

templatesdir = $(datadir)/$(PACKAGE)/templates
skeletonsdir = $(datadir)/$(PACKAGE)/templates/skeletons
templatessrcdir = $(top_srcdir)/data/extra/templates

templates_DATA = \
eflproject.edj \
eflproject_python.edj

skeletons_DATA = \
eflproject.tar.gz \
eflproject_python.tar.gz

eflproject.tar.gz:
	tar zcvf eflproject.tar.gz -C $(templatessrcdir)/skeletons eflproject

eflproject_python.tar.gz:
	tar zcvf eflproject_python.tar.gz -C $(templatessrcdir)/skeletons eflproject_python

eflproject.edj:
	edje_cc -id $(templatessrcdir)/images/ $(templatessrcdir)/eflproject.edc

eflproject_python.edj:
	edje_cc -id $(templatessrcdir)/images/ $(templatessrcdir)/eflproject_python.edc

EXTRA_DIST = $(templates_DATA) $(skeletons_DATA)
