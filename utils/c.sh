#!/usr/bin/env sh

if [[ ! -f "CMakeLists.txt" ]]; then
    cd ..
    if [[ ! -f "CMakeLists.txt" ]]; then
        "$PWD does not contain 'CMakeLists.txt'"
        exit 1
    fi
fi

cmake . && make && sudo -E ./iftop $@
./utils/clean.sh
