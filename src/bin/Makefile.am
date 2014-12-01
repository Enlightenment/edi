MAINTAINERCLEANFILES = Makefile.in
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = edi edi_build

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-I$(top_builddir)/src/bin/ \
-I$(top_srcdir)/src/bin/ \
-I$(top_srcdir)/elm_code/lib \
-I$(top_builddir)/src/lib/ \
-DEFL_BETA_API_SUPPORT \
@EFL_CFLAGS@

edi_SOURCES = \
edi_config.c \
editor/edi_editor_search.c \
editor/edi_editor.c \
edi_content_provider.c \
welcome/edi_welcome.c \
edi_filepanel.c \
edi_logpanel.c \
edi_consolepanel.c \
mainview/edi_mainview_item.c \
mainview/edi_mainview.c \
edi_main.c

edi_LDADD = @EFL_LIBS@ $(top_builddir)/elm_code/lib/libelm_code.la $(top_builddir)/src/lib/libedi.la

edi_build_SOURCES = \
edi_build_main.c

edi_build_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/libedi.la

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

EXTRA_DIST = edi_private.h

