MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = edi edi_build

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-I$(top_builddir)/src/bin/ \
-I$(top_srcdir)/src/bin/ \
-I$(top_builddir)/src/lib/ \
-I$(top_srcdir)/src/lib/ \
@EFL_CFLAGS@

edi_SOURCES = \
edi_filepanel.c \
edi_logpanel.c \
edi_consolepanel.c \
edi_mainview.c \
edi_main.c

edi_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/libedi.la

edi_build_SOURCES = \
edi_build_main.c

edi_build_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/libedi.la

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

EXTRA_DIST = edi_private.h

