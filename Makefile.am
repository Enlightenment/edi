MAINTAINERCLEANFILES = \
	ABOUT-NLS \
	Makefile.in \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.rpath \
	config.sub \
	configure \
	depcomp \
	install-sh \
	ltmain.sh \
	missing \
	m4/intl.m4 \
	m4/intldir.m4 \
	m4/intlmacosx.m4 \
	m4/libtool.m4 \
	m4/ltoptions.m4 \
	m4/ltsugar.m4 \
	m4/ltversion.m4 \
	m4/lt~obsolete.m4 \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/Makefile.in.in \
	po/Makevars.template \
	po/quot.sed \
	po/remove-potcdate.sin \
	po/Rules-quot \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.bz2

SUBDIRS = data doc packaging po src

ACLOCAL_AMFLAGS = -I m4

filesdir = $(docdir)
files_DATA = \
	AUTHORS \
	COPYING \
	COPYING.LGPL \
	COPYING.GPLv2 \
	ChangeLog \
	INSTALL \
	NEWS \
	README \
	TODO

EXTRA_DIST = \
	autogen.sh \
	COPYING.LGPL \
	COPYING.GPLv2 \
	config.rpath \
	configure \
	m4/efl_binary.m4 \
	m4/efl_compiler_flag.m4 \
	m4/efl_path_max.m4

if EFL_HAVE_LCOV

lcov-reset:
	@rm -rf $(top_builddir)/coverage
	@find $(top_builddir) -name "*.gcda" -delete
	@lcov --zerocounters --directory $(top_builddir)

lcov-report:
	@mkdir $(top_builddir)/coverage
	lcov --capture --compat-libtool --output-file $(top_builddir)/coverage/coverage.info --directory $(top_builddir)/src/lib --base-directory $(top_srcdir)/src/lib
	lcov --remove $(top_builddir)/coverage/coverage.info '*.h' --output-file $(top_builddir)/coverage/coverage.cleaned.info
	lcov --remove $(top_builddir)/coverage/coverage.cleaned.info '*/tests/*' --output-file $(top_builddir)/coverage/coverage.cleaned2.info
	genhtml --branch-coverage -t "$(PACKAGE_STRING)" -o $(top_builddir)/coverage/html $(top_builddir)/coverage/coverage.cleaned2.info
	@echo "Coverage Report at $(top_builddir)/coverage/html"

endif

if EFL_HAVE_TESTS

TESTS = src/tests/edi_suite

lcov-check:
if EFL_HAVE_LCOV
	@$(MAKE) $(AM_MAKEFLAGS) lcov-reset
endif
	@$(MAKE) $(AM_MAKEFLAGS) check
if EFL_HAVE_LCOV
	@$(MAKE) $(AM_MAKEFLAGS) lcov-report
endif

endif

# cleaning

clean-local:
	rm -rf benchmark coverage

