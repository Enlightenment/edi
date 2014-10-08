EFL_VERSION([0], [0], [1], [dev])
AC_INIT([edi], [efl_version], [enlightenment-devel@lists.sourceforge.net])

AC_PREREQ([2.65])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.10 dist-bzip2 -Wall color-tests])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_PROG_AR

# this will speed up libtool checks
LT_PREREQ([2.2])
LT_INIT([win32-dll pic-only])
EFL_INIT

AC_PROG_CC
AM_PROG_CC_C_O
EFL_ATTRIBUTE_UNUSED
ELM_QUICKLAUNCH
EFL_COMPILER_FLAG([-Wall])
EFL_COMPILER_FLAG([-W])

# Checks for header files.
EFL_CHECK_PATH_MAX

# Check for EFL
PKG_CHECK_MODULES([EFL],
[
 eina >= 1.8.0
 evas >= 1.8.0
 ecore >= 1.8.0
 edje >= 1.8.0
 elementary >= 1.8.0
 eio >= 1.8.0
])

# Check for tests and coverage
AC_ARG_WITH([tests],
   [AC_HELP_STRING([--with-tests=none|regular|coverage],
                   [choose testing method: regular, coverage or none.
                    @<:@default=none@:>@])],
   [build_tests=${withval}],
   [build_tests=auto])


HAVE_LIBCLANG=0

AC_ARG_ENABLE(libclang,
      [  --disable-libclang	Disable use of libclang.],,)

build_clang="no"
if test "$enable_libclang" != "no"; then
  AC_CHECK_HEADERS("clang-c/Index.h")
  if test "$ac_cv_header_clang_c_Index_h" = "yes"; then
    AC_CHECK_LIB(clang, clang_Cursor_getArgument)
    if test "$ac_cv_lib_clang_clang_Cursor_getArgument" = "yes"; then
      HAVE_LIBCLANG=1;
      MORE_LIBS="$MORE_LIBS -lclang"
      build_clang="yes"
    fi
  fi 
  if test "$HAVE_LIBCLANG" = "0"; then
    AC_MSG_WARN([Could not find a usable libclang. Please install libclang to enable syntax highlighting features.])
  fi
fi

AC_SUBST(HAVE_LIBCLANG)

EFL_TESTS([${build_tests}])

# doxygen program for documentation building
EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])

# Check edje_cc
EFL_WITH_BIN([edje], [edje-cc], [edje_cc])

# Checks for library functions.
AC_CHECK_FUNCS([setlocale])

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

AC_CONFIG_FILES([
Makefile
data/Makefile
data/images/Makefile
data/desktop/Makefile
data/desktop/edi.desktop
data/themes/Makefile
data/themes/default/Makefile
data/extra/Makefile
doc/Makefile
packaging/Makefile
packaging/pkgbuild/Makefile
po/Makefile.in
src/Makefile
src/bin/Makefile
src/lib/Makefile
src/tests/Makefile
doc/edi.1
])
AC_OUTPUT

echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE_NAME $PACKAGE_VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Compilation..............: make (or gmake)"
echo "  CFLAGS.................: $CFLAGS"
echo "  edje_cc................: ${edje_cc}"
echo "  clang..................: ${build_clang}"
echo
echo "Building documentation...: ${build_doc}"
echo "Building tests...........: ${have_tests}"
echo "Generate coverage .......: ${have_lcov}"
echo
echo "Installation.............: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix.................: $prefix"
echo
