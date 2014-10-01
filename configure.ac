
AC_PREREQ([2.69])
AC_INIT([ibus-rtk], m4_esyscmd_s(echo 1.$(git rev-list HEAD --count)),
    [lordyomin@hivegate.net], [ibus-rtk], [http://github.com/Yomin/ibus-rtk])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# check ibus
PKG_CHECK_MODULES([IBUS], [ibus-1.0 >= 1.3.0])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

AC_CHECK_PROGS([DLPROG], [git wget curl])
if test "x$ac_cv_prog_DLPROG" = x; then
    AC_MSG_ERROR([neither git nor wget nor curl available])
fi
AC_CHECK_PROG([TAR], [tar], [tar])
if test "x$ac_cv_prog_TAR" = x; then
    AC_MSG_ERROR([tar not avaiable])
fi

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
Makefile
icons/Makefile
dicts/Makefile
src/Makefile
ibus-rtk.spec
])

AC_OUTPUT
