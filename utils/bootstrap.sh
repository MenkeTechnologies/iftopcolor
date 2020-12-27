#!/bin/sh
#
# bootstrap:
# Build the configure script from the .in files.
#
# $Id: bootstrap,v 1.1 2002/11/04 12:27:35 chris Exp $
#

if [[ ! -f "Makefile.am" ]]; then
    cd ..
    if [[ ! -f "Makefile.am" ]]; then
        "$PWD does not contain 'Makefile.am'"
        exit 1
    fi
fi

set -x
aclocal -I config
autoheader
autoconf
automake
