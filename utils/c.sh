#!/usr/bin/env sh

if [[ ! -f "Makefile.am" ]]; then
    cd ..
    if [[ ! -f "Makefile.am" ]]; then
        "$PWD does not contain 'Makefile.am'"
        exit 1
    fi
fi

cmake . && make && sudo -E ./iftop $@
./utils/clean.sh
