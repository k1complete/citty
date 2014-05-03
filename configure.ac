#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([citty], [0.0.1], [k1complete@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/citty.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([lib])
AC_CONFIG_FILES([Makefile src/Makefile])
# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_CHECK_FUNCS([memset regcomp select])
AC_CHECK_FUNCS([strlcpy])
AC_LIBOBJ([strlcpy])
AC_CHECK_FUNCS([forkpty])

AC_OUTPUT
