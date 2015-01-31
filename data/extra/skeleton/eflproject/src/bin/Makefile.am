MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = ${edi_name}

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-I$(top_builddir)/src/bin/ \
-I$(top_srcdir)/src/bin/ \
-I$(top_builddir)/src/lib/ \
-I$(top_srcdir)/src/lib/ \
@EFL_CFLAGS@

${edi_name}_SOURCES = ${edi_name}_main.c
${edi_name}_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/lib${edi_name}.la

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

EXTRA_DIST = ${edi_name}_private.h

