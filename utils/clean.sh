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
        config.h.in \
        configure \
        Makefile.in \
        autom4te.cache \