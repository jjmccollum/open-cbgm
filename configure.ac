# configure.ac
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([open-cbgm], [1.0], [jjmccollum@vt.edu])
AC_CONFIG_SRCDIR([src/open_cbgm.cpp])
AC_LANG(C++)

# Check for the C++ compiler used on this platform:
AC_PROG_CXX

# Checks for support for standard header files:
AC_CHECK_HEADERS([stdlib.h stdio.h getopt.h string.h wchar.h float.h stdint.h inttypes.h limits.h stddef.h], [], [AC_MSG_ERROR(Could not use standard headers)])

# Check for typedefs, structures, and compiler characteristics:
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Check for library functions:
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor memmove memset select strchr strstr])

# Initialize automake:
#AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
