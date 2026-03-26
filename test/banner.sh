#!/bin/sh
# Cyberpunk banner for test runner

C='\033[1;36m'  # bold cyan
G='\033[1;32m'  # bold green
B='\033[1m'     # bold
R='\033[0m'     # reset
M='\033[1;35m'  # bold magenta
Y='\033[1;33m'  # bold yellow
D='\033[2m'     # dim

COUNTS_FILE="/tmp/iftop_test_counts"

case "$1" in
    start)
        rm -f "$COUNTS_FILE"
        printf '\n'
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf "  ${C}░░${R}  ${B}DEPLOYING DIAGNOSTIC ROUTINES // INTEGRITY CHECK${R}              ${C}░░${R}\n"
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
        printf '\n'
        ;;
    end)
        # Aggregate counts from all test binaries
        total_pass=0
        total_fail=0
        total_tests=0
        num_suites=0
        if [ -f "$COUNTS_FILE" ]; then
            while read -r p f t; do
                total_pass=$((total_pass + p))
                total_fail=$((total_fail + f))
                total_tests=$((total_tests + t))
                num_suites=$((num_suites + 1))
            done < "$COUNTS_FILE"
            rm -f "$COUNTS_FILE"
        fi

        printf '\n'
        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"

        if [ "$total_fail" -eq 0 ] 2>/dev/null; then
            printf "  ${C}░░${R}  ${G}▓▓ ALL SUBSYSTEMS VERIFIED // 0 BREACHES DETECTED${R}            ${C}░░${R}\n"
        else
            printf "  ${C}░░${R}  ${M}▓▓ INTEGRITY BREACH // %d FAILURES DETECTED${R}                  ${C}░░${R}\n" "$total_fail"
        fi

        printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"

        if [ "$total_tests" -gt 0 ]; then
            printf '\n'
            printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
            printf "  ${M}║${R}  ${Y}▒▒▒${R} ${B}G R A N D   T O T A L${R}                                          ${M}║${R}\n"
            printf "  ${M}╠══════════════════════════════════════════════════════════════════════╣${R}\n"
            printf "  ${M}║${R}                                                                    ${M}║${R}\n"
            printf "  ${M}║${R}   ${C}░ SUITES EXECUTED${R}  ${D}.....................${R}  ${B}%-4d${R}                     ${M}║${R}\n" "$num_suites"
            printf "  ${M}║${R}   ${C}░ TOTAL TESTS${R}      ${D}.....................${R}  ${B}%-4d${R}                     ${M}║${R}\n" "$total_tests"
            printf "  ${M}║${R}   ${G}░ PASSED${R}           ${D}.....................${R}  ${G}%-4d${R}                     ${M}║${R}\n" "$total_pass"
            if [ "$total_fail" -gt 0 ]; then
                printf "  ${M}║${R}   ${M}░ FAILED${R}           ${D}.....................${R}  ${M}%-4d${R}                     ${M}║${R}\n" "$total_fail"
            else
                printf "  ${M}║${R}   ${G}░ FAILED${R}           ${D}.....................${R}  ${G}%-4d${R}                     ${M}║${R}\n" "$total_fail"
            fi
            printf "  ${M}║${R}                                                                    ${M}║${R}\n"
            printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        fi
        printf '\n'
        ;;
esac
