dnl
dnl configure.in:
dnl Autoconf input for iftop.
dnl
dnl I hate autoconf with a passion. It's an utter pain to write these bloody
dnl things and even once you have you find yourself testing for more and more
dnl special cases. But that's OK. Paul is going to maintain it :)
dnl     -- Chris Lightfoot
dnl
dnl $Id: configure.in,v 1.33 2011/10/03 21:55:33 pdw Exp $
dnl
dnl To regenerate everything from source, do:
dnl    autoheader
dnl    aclocal
dnl    automake
dnl    autoconf
dnl Now you should have good sources to make into a tarball and distribute.
dnl    ./configure     (perhaps with some arguments)
dnl    make
dnl Tested with Automake 1.4 and autoconf 2.59.
dnl
dnl Boilerplate configuration
dnl

AC_INIT([iftop], [2020.12.26])
AC_CONFIG_SRCDIR([src/iftop.c])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
