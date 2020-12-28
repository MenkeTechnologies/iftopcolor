#!/usr/bin/env sh

if [[ ! -f "CMakeLists.txt" ]]; then
    cd ..
    if [[ ! -f "CMakeLists.txt" ]]; then
        "$PWD does not contain 'CMakeLists.txt'"
        exit 1
    fi
fi

set -x
rm -rf \
        aclocal.m4 \
        src/*.o \
        iftop \
        cmake_install.cmake \
        CMakeCache.txt \
        src/iftop \
        iftop.spec \
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
