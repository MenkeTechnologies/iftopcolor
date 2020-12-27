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
        src/*.o \
        src/.deps \
        .deps \
        src/Makefile \
        src/Makefile.in \
        src/autoscan.log \
        src/configure.scan \
        autoscan.log \
        configure.scan \
        config.h.in \
        compile \
        missing \
        install-sh \
        stamp-* \
        config.h \
        config.log \
        config.status \
        Makefile \
        configure \
        depcomp \
        Makefile.in \
        autom4te.cache \