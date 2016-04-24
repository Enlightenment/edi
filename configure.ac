EFL_VERSION([0], [2], [0])
AC_INIT([edi], [efl_version], [enlightenment-devel@lists.sourceforge.net])

AC_PREREQ([2.65])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.10 dist-bzip2 -Wall color-tests subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


### Additional options to configure

# Check for tests and coverage
AC_ARG_WITH([tests],
   [AS_HELP_STRING([--with-tests=none|regular|coverage],
                   [choose testing method: regular, coverage or none.
                    @<:@default=auto@:>@])],
   [build_tests=${withval}],
   [build_tests=auto])

# clang support
AC_ARG_ENABLE([libclang],
   [AS_HELP_STRING([--disable-libclang], [Disable use of libclang. @<:@default=enable@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       want_clang="yes"
    else
       want_clang="no"
    fi
   ],
   [want_clang="yes"])


### Checks for programs

LT_PREREQ([2.2])
LT_INIT([win32-dll pic-only])
EFL_INIT

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

# if cross compiling, disable NLS support.
# It's not worth the trouble, at least for now.
if test "x$cross_compiling" = xyes; then
  enable_nls=no
fi

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_CC

PKG_PROG_PKG_CONFIG

EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])


### Checks for libraries

PKG_CHECK_MODULES([EFL],
[
 eina >= 1.17.0
 evas >= 1.17.0
 ecore >= 1.17.0
 edje >= 1.17.0
 eo >= 1.17.0
 elementary >= 1.17.99
 eio >= 1.17.0
])

# Check edje_cc
EFL_WITH_BIN([edje], [edje-cc], [edje_cc])
EFL_WITH_BIN([eolian], [eolian-gen], [eolian_gen])
# Force the helper to try external eolian generators
AM_CONDITIONAL([HAVE_EOLIAN_GEN], [true])

# Needs to be moved into a macro, and also, needs a way to automatically fetch
# from all the dependencies using the Requires.
DEPS_EOLIAN_FLAGS=`${PKG_CONFIG} --variable=eolian_flags eo evas edje efl elementary`
AC_SUBST([DEPS_EOLIAN_FLAGS])


### Checks for header files


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AM_PROG_CC_C_O

EFL_CHECK_PATH_MAX
EFL_COMPILER_FLAG([-Wall])
EFL_COMPILER_FLAG([-Wextra])
EFL_COMPILER_FLAG([-fvisibility=hidden])


### Checks for linker characteristics


### Checks for library functions

AC_CHECK_FUNCS([setlocale])

# this will speed up libtool checks


# Checks for header files.

have_clang="no"
if test "${want_clang}" = "yes"; then
  AC_CHECK_HEADERS([clang-c/Index.h], [have_clang="yes"], [have_clang="no"])
fi

EFL_TESTS([${build_tests}])

# Checks for library functions.

if test "${have_clang}" = "yes"; then
   ldflags="$LDFLAGS"
   LDFLAGS+=" -L${libdir}/llvm"
   AC_CHECK_LIB([clang], [clang_Cursor_getArgument],
      [
       AC_DEFINE([HAVE_LIBCLANG], [1], [Set to 1 if clang is found])
       EFL_LIBS="${EFL_LIBS} -lclang"
      ],
      [have_clang="no"])
fi

if test "${have_clang}" = "no"; then
   AC_MSG_WARN([Could not find a usable libclang. Please install libclang to enable syntax highlighting features.])
fi
CLANG_INCLUDE=`clang -E - -v < /dev/null 2>&1 | grep "^ /usr" | grep clang`
AC_SUBST([CLANG_INCLUDE])

AC_CONFIG_FILES([
Makefile
data/Makefile
data/images/Makefile
data/desktop/Makefile
data/desktop/edi.desktop
data/themes/Makefile
data/themes/default/Makefile
data/extra/Makefile
data/extra/skeleton/Makefile
doc/Makefile
packaging/Makefile
packaging/pkgbuild/Makefile
po/Makefile.in
src/Makefile
src/bin/Makefile
src/lib/Makefile
src/tests/Makefile
elm_code/Makefile
elm_code/data/Makefile
elm_code/data/themes/Makefile
elm_code/data/themes/default/Makefile
elm_code/src/Makefile
elm_code/src/lib/Makefile
elm_code/src/lib/widget/Makefile
elm_code/src/bin/Makefile
elm_code/src/tests/Makefile
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
echo "  CPPFLAGS...............: $CPPFLAGS"
echo "  CFLAGS.................: $CFLAGS"
echo "  LDFLAGS................: $LDFLAGS"
echo "  edje_cc................: ${edje_cc}"
echo "  highlighting (libclang): ${have_clang}"
echo
echo "eolian_gen...............: ${eolian_gen}"
echo
echo "Building documentation...: ${build_doc}"
echo "Building tests...........: ${have_tests}"
echo "Generate coverage .......: ${have_lcov}"
echo
echo "Installation.............: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix.................: $prefix"
echo
