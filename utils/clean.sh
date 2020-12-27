#!/bin/sh

if [[ ! -f "Makefile.am" ]]; then
    cd ..
    if [[ ! -f "Makefile.am" ]]; then
        "$PWD does not contain 'Makefile.am'"
        exit 1
    fi
fi

set -x
rm -rf \
        aclocal.m4 \
        src/autoscan.log \
        src/configure.scan \
        src/configure.ac \
        autoscan.log \
        configure.scan \
        configure.ac \
        config.h.in \
        config.h \
        config.log \
        config.status \
        Makefile \
        configure \
        depcomp \
        Makefile.in \
        autom4te.cache \