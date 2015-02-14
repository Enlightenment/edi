MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = check-results.xml

if EFL_HAVE_TESTS

check_PROGRAMS = edi_suite

edi_suite_SOURCES = \
edi_test_path.c \
edi_test_content_provider.c \
edi_suite.c

edi_suite_CPPFLAGS = -I$(top_builddir)/src/lib/ -I$(top_builddir)/src/bin/ \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/elm_code/src/lib \
-I$(top_srcdir)/src/bin \
-DPACKAGE_TESTS_DIR=\"$(top_srcdir)/src/tests/\" \
-DPACKAGE_BUILD_DIR=\"`pwd`/$(top_builddir)/src/tests/\" \
-DEFL_BETA_API_SUPPORT \
@EFL_CFLAGS@ \
@CHECK_CFLAGS@

edi_suite_LDADD = @EFL_LIBS@ @CHECK_LIBS@ $(top_builddir)/src/lib/libedi.la $(top_builddir)/elm_code/src/lib/libelm_code.la
edi_suite_DEPENDENCIES = $(top_builddir)/src/lib/libedi.la $(top_builddir)/elm_code/src/lib/libelm_code.la

endif

EXTRA_DIST = edi_suite.h
