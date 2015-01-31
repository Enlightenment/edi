
if EFL_HAVE_TESTS

check_PROGRAMS = ${edi_name}_tests

${edi_name}_tests_SOURCES = test_${edi_name}.c
${edi_name}_tests_CPPFLAGS = -I$(top_builddir)/src/lib/ \
-DPACKAGE_TESTS_DIR=\"$(top_srcdir)/src/tests/\" \
-DPACKAGE_BUILD_DIR=\"`pwd`/$(top_builddir)/src/tests/\" \
@EFL_CFLAGS@ \
@CHECK_CFLAGS@
${edi_name}_tests_LDADD = @CHECK_LIBS@ $(top_builddir)/src/lib/lib${edi_name}.la
${edi_name}_tests_DEPENDENCIES = $(top_builddir)/src/lib/lib${edi_name}.la

endif

EXTRA_DIST = test_${edi_name}.c
