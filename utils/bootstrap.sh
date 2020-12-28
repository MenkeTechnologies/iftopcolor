#!/usr/bin/env sh
#
# bootstrap:
# Build the configure script from the .in files.
#
# $Id: bootstrap,v 1.1 2002/11/04 12:27:35 chris Exp $
#

if [[ ! -f "CMakeLists.txt" ]]; then
    cd ..
    if [[ ! -f "CMakeLists.txt" ]]; then
        "$PWD does not contain 'CMakeLists.txt'"
        exit 1
    fi
fi

set -x
aclocal -I config
autoheader
autoreconf --install
autoconf
automake --add-missing
