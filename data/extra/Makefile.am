MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/
skeleton_DATA =

install-data-hook:
	cp -r skeleton $(skeletondir)

uninstall-local:
	rm -rf $(skeletondir)/skeleton


EXTRA_DIST = $(skeleton_DATA)
