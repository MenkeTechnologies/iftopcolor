#!/usr/bin/env sh

if [[ ! -f "CMakeLists.txt" ]]; then
    cd ..
    if [[ ! -f "CMakeLists.txt" ]]; then
        "$PWD does not contain 'CMakeLists.txt'"
        exit 1
    fi
fi

./utils/clean.sh
./utils/bootstrap.sh
./configure
make
