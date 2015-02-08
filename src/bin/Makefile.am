MAINTAINERCLEANFILES = Makefile.in
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = edi edi_build

AM_CPPFLAGS = \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DPACKAGE_DOC_DIR=\"$(docdir)\" \
-DLOCALEDIR=\"$(datadir)/locale\" \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/elm_code/lib \
-I$(top_builddir)/src/lib \
-DEFL_BETA_API_SUPPORT \
@EFL_CFLAGS@

noinst_HEADERS = \
edi_config.h \
editor/edi_editor.h \
edi_content_provider.h \
screens/edi_screens.h \
edi_filepanel.h \
edi_logpanel.h \
edi_consolepanel.h \
mainview/edi_mainview_item.h \
mainview/edi_mainview.h

edi_SOURCES = \
edi_config.c \
editor/edi_editor_search.c \
editor/edi_editor.c \
edi_content_provider.c \
screens/edi_welcome.c \
screens/edi_about.c \
edi_filepanel.c \
edi_logpanel.c \
edi_consolepanel.c \
mainview/edi_mainview_item.c \
mainview/edi_mainview.c \
edi_main.c \
edi_private.h

edi_LDADD = @EFL_LIBS@ $(top_builddir)/elm_code/lib/libelm_code.la $(top_builddir)/src/lib/libedi.la

edi_build_SOURCES = \
edi_build_main.c

edi_build_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/libedi.la

