#!/bin/sh
# Cyberpunk banner for benchmark runner

C='\033[1;36m'  # bold cyan
G='\033[1;32m'  # bold green
B='\033[1m'     # bold
R='\033[0m'     # reset

case "$1" in
    start)
        printf '\n'
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf "  ${C}░░${R}  ${B}INITIALIZING PERFORMANCE PROFILER // MAXIMUM OVERDRIVE${R}        ${C}░░${R}\n"
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf '\n'
        ;;
    end)
        printf '\n'
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf "  ${C}░░${R}  ${G}▓▓ ALL BENCHMARKS COMPLETE // CONNECTION TERMINATED${R}          ${C}░░${R}\n"
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf '\n'
        ;;
esac
