MAINTAINERCLEANFILES = Makefile.in
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = edi edi_build
efl_cflags_safe = '@EFL_CFLAGS@'
clang_include = '$(CLANG_INCLUDE)'

AM_CPPFLAGS = \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DPACKAGE_DOC_DIR=\"$(docdir)\" \
-DVERSION=\"$(PACKAGE_VERSION)\" \
-DLOCALEDIR=\"$(datadir)/locale\" \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/bin \
-I$(top_builddir)/src/lib \
-DEFL_BETA_API_SUPPORT \
-DEFL_CFLAGS=\"$(efl_cflags_safe)\" \
-DCLANG_INCLUDES=\"-I$(clang_include)\" \
@EFL_CFLAGS@

noinst_HEADERS = \
edi_config.h \
editor/edi_editor.h \
editor/edi_editor_suggest_provider.h \
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
editor/edi_editor_suggest_provider.c \
editor/edi_editor.c \
edi_content_provider.c \
screens/edi_welcome.c \
screens/edi_about.c \
screens/edi_settings_font.c \
screens/edi_settings.c \
edi_filepanel.c \
edi_logpanel.c \
edi_consolepanel.c \
mainview/edi_mainview_item.c \
mainview/edi_mainview.c \
edi_main.c \
edi_private.h

edi_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/libedi.la $(LTLIBINTL)

edi_build_SOURCES = \
edi_build_main.c

edi_build_LDADD = @EFL_LIBS@ $(top_builddir)/src/lib/libedi.la

