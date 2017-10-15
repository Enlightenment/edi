dnl Copyright (C) 2013 Cedric BAIL <cedric.bail at free dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Macro for checking availability of tests and coverage infra structure

dnl Usage: EFL_TESTS(profile)
dnl Valid profile are auto, tests, coverage, no
dnl Call PKG_CHECK_MODULES, AC_CHECK_PROG, define CHECK_CFLAGS/CHECK_LIBS and modify CFLAGS/LIBS
dnl It define EFL_HAVE_TESTS/EFL_HAVE_LCOV for use in Makefile.am
dnl It set have_test and have_coverage to yes/no depending if found

AC_DEFUN([EFL_TESTS],
[
build_tests=$1

case "${build_tests}" in
     auto)
	check_tests="auto"
	check_coverage="auto"
	;;
     tests)
	check_tests="yes"
	check_coverage="auto"
	;;
     coverage)
	check_tests="yes"
	check_coverage="yes"
        ;;
     no)
	check_tests="no"
	check_coverage="no"
	;;
     *)
	AC_MSG_ERROR([Unknow tests profile])
esac

have_tests="no"
if test "x${check_tests}" = "xyes" -o "x${check_tests}" = "xauto"; then
   PKG_CHECK_MODULES([CHECK], [check >= 0.9.5], [have_tests="yes"], [have_tests="no"])
   if test "${check_tests}" = "xyes" -a "x${have_tests}" = "xno"; then
      AC_MSG_ERROR([Impossible to find check package to build tests])
   fi
fi

if test "x${have_tests}" = "xyes"; then
   if test "x${check_coverage}" = "xyes" -o "x${check_coverage}" = "xauto"; then
      AC_CHECK_PROG([have_lcov], [lcov], [yes], [no])
      if test "x${have_lcov}" = "xyes" ; then
      	 CFLAGS="${CFLAGS} -fprofile-arcs -ftest-coverage"
	 LIBS="${LIBS} -lgcov"
      fi
      if test "x${have_lcov}" = "xno" -a "x${check_coverage}" = "xyes"; then
      	 AC_MSG_ERROR([Impossible to find lcov package to build with coverage support])
      fi
   else
      have_coverage="no"
   fi
else
   have_coverage="no"
fi

AM_CONDITIONAL([EFL_HAVE_TESTS], [test "x${have_tests}" = "xyes"])
AM_CONDITIONAL([EFL_HAVE_LCOV], [test "x${have_lcov}" = "xyes"])

])
