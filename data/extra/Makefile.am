MAINTAINERCLEANFILES = Makefile.in

skeletondir = $(datadir)/$(PACKAGE)/
skeleton_DATA = skeleton/README

install-data-hook:
## removing the temporary README that is used for a git submodule check
	rm -f $(skeletondir)/README
	cp -rv skeleton $(skeletondir)

uninstall-local:
	rm -rf $(skeletondir)/skeleton

EXTRA_DIST = $(skeleton_DATA)
