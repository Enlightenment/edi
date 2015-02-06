MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/
skeleton_DATA =

install-data-hook:
	cp -r $(srcdir)/skeleton $(skeletondir)

uninstall-local:
	rm -rf $(skeletondir)/skeleton

EXTRA_DIST = skeleton $(skeleton_DATA)
