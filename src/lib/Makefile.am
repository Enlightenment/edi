MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EFL_CFLAGS@ \
-DEFL_EDI_BUILD

lib_LTLIBRARIES = libedi.la

includes_HEADERS = \
edi_builder.h \
edi_create.h \
edi_path.h \
Edi.h
includesdir = $(includedir)/edi-@VMAJ@

libedi_la_SOURCES = \
edi_builder.c \
edi_create.c \
edi_path.c \
edi.c
libedi_la_LIBADD = @EFL_LIBS@ -lm
libedi_la_LDFLAGS = -no-undefined @EFL_LTLIBRARY_FLAGS@
