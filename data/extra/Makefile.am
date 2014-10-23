MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/
skeleton_DATA = 

install-data-hook:
	tar -xf $(srcdir)/skeleton.tar.gz -C $(skeletondir)

uninstall-local:
	rm -rf $(skeletondir)/skeleton

EXTRA_DIST = $(skeleton_DATA)
