#!/bin/sh
# Cyberpunk banner for test runner

C='\033[1;36m'  # bold cyan
G='\033[1;32m'  # bold green
B='\033[1m'     # bold
R='\033[0m'     # reset

case "$1" in
    start)
        printf '\n'
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf "  ${C}░░${R}  ${B}DEPLOYING DIAGNOSTIC ROUTINES // INTEGRITY CHECK${R}              ${C}░░${R}\n"
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf '\n'
        ;;
    end)
        printf '\n'
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf "  ${C}░░${R}  ${G}▓▓ ALL SUBSYSTEMS VERIFIED // 0 BREACHES DETECTED${R}            ${C}░░${R}\n"
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf '\n'
        ;;
esac
