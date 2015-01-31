MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EFL_CFLAGS@ \
-DEFL_EFL_BUILD

lib_LTLIBRARIES = lib${edi_name}.la

includes_HEADERS = ${EDI_NAME}.h
includesdir = $(includedir)/${edi_name}-@VMAJ@

lib${edi_name}_la_SOURCES = ${edi_name}.c
lib${edi_name}_la_LIBADD = @EFL_LIBS@ -lm
lib${edi_name}_la_LDFLAGS = @EFL_LTLIBRARY_FLAGS@
