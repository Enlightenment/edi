
if EFL_HAVE_TESTS

check_PROGRAMS = edi_tests

edi_tests_SOURCES = test_edi.c
edi_tests_CPPFLAGS = -I$(top_builddir)/src/lib/ \
-DPACKAGE_TESTS_DIR=\"$(top_srcdir)/src/tests/\" \
-DPACKAGE_BUILD_DIR=\"`pwd`/$(top_builddir)/src/tests/\" \
@EFL_CFLAGS@ \
@CHECK_CFLAGS@
edi_tests_LDADD = @CHECK_LIBS@ $(top_builddir)/src/lib/libedi.la
edi_tests_DEPENDENCIES = $(top_builddir)/src/lib/libedi.la

endif

EXTRA_DIST = test_edi.c
