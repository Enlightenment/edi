MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EFL_CFLAGS@ \
-DEFL_EDI_BUILD \
-DBEAR_COMMAND=\"$(BEAR_COMMAND)\"

lib_LTLIBRARIES = libedi.la

includes_HEADERS = \
edi_build_provider.h \
edi_builder.h \
edi_create.h \
edi_path.h \
edi_exe.h \
Edi.h
includesdir = $(includedir)/edi-@VMAJ@

libedi_la_SOURCES = \
edi_private.h \
edi_build_provider_make.c \
edi_build_provider_cmake.c \
edi_build_provider_python.c \
edi_build_provider.c \
edi_builder.c \
edi_create.c \
edi_path.c \
edi_exe.c \
edi.c
libedi_la_LIBADD = @EFL_LIBS@ -lm
libedi_la_LDFLAGS = -no-undefined @EFL_LTLIBRARY_FLAGS@
