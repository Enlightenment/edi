AUTOMAKE_OPTIONS = subdir-objects
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
default.edc \
images/enlightenment.png \
sounds/bell.wav

include ../../../Makefile_Edje_Helper.am

filesdir = $(datadir)/$(PACKAGE)/themes
files_DATA = default.edj

default.edj: Makefile $(EXTRA_DIST)
	$(AM_V_EDJ)$(EDJE_CC) $(EDJE_CC_FLAGS) \
	-id ${top_srcdir}/data/themes/default/images \
	-sd ${top_srcdir}/data/themes/default/sounds \
	$(top_srcdir)/data/themes/default/default.edc \
	$(top_builddir)/data/themes/default/default.edj

clean-local:
	rm -f *.edj
