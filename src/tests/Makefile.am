
if EFL_HAVE_TESTS

check_PROGRAMS = edi_suite

edi_suite_SOURCES = \
edi_test_path.c \
edi_test_console.c \
edi_suite.c

edi_suite_CPPFLAGS = -I$(top_builddir)/src/lib/ -I$(top_builddir)/src/bin/ \
-DPACKAGE_TESTS_DIR=\"$(top_srcdir)/src/tests/\" \
-DPACKAGE_BUILD_DIR=\"`pwd`/$(top_builddir)/src/tests/\" \
@EFL_CFLAGS@ \
@CHECK_CFLAGS@

edi_suite_LDADD = @CHECK_LIBS@ $(top_builddir)/src/lib/libedi.la
edi_suite_DEPENDENCIES = $(top_builddir)/src/lib/libedi.la

endif

EXTRA_DIST = edi_suite.h
