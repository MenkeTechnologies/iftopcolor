#!/usr/bin/env sh

if [[ ! -f "Makefile.am" ]]; then
    cd ..
    if [[ ! -f "Makefile.am" ]]; then
        "$PWD does not contain 'Makefile.am'"
        exit 1
    fi
fi

./utils/autogen.sh && sudo -E ./src/iftop $@
./utils/clean.sh
