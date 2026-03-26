#!/bin/sh
# run_check.sh — run all test binaries, tally results, print cyberpunk summary

# Colors
C='\033[1;36m'   # bold cyan
G='\033[1;32m'   # bold green
R='\033[0m'      # reset
B='\033[1m'      # bold
M='\033[1;35m'   # bold magenta
Y='\033[1;33m'   # bold yellow
RED='\033[1;31m' # bold red
DIM='\033[2m'    # dim

# Start banner
printf '\n'
printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
printf "  ${C}░░${R}  ${B}DEPLOYING DIAGNOSTIC ROUTINES // INTEGRITY CHECK${R}              ${C}░░${R}\n"
printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"
printf '\n'

total_passed=0
total_failed=0
total_tests=0
num_suites=0
any_failed=0

for bin in "$@"; do
    output=$("$bin" 2>&1)
    rc=$?
    printf '%s\n' "$output"

    # Parse: "// 42 tests passed //" or "// 40 passed, 2 FAILED, 42 total"
    passed=$(printf '%s' "$output" | sed -n 's/.*\/\/ \([0-9]*\) tests passed.*/\1/p')
    if [ -n "$passed" ]; then
        total_passed=$((total_passed + passed))
        total_tests=$((total_tests + passed))
    else
        p=$(printf '%s' "$output" | sed -n 's/.*\/\/ \([0-9]*\) passed,.*/\1/p')
        f=$(printf '%s' "$output" | sed -n 's/.*, \([0-9]*\) FAILED,.*/\1/p')
        [ -n "$p" ] && total_passed=$((total_passed + p))
        [ -n "$f" ] && total_failed=$((total_failed + f))
        [ -n "$p" ] && [ -n "$f" ] && total_tests=$((total_tests + p + f))
    fi

    num_suites=$((num_suites + 1))
    [ $rc -ne 0 ] && any_failed=1
done

# Helper: pad a string to exactly N visible columns with trailing spaces
pad() {
    _str="$1"
    _want="$2"
    _blen=$(printf '%s' "$_str" | wc -c | tr -d ' ')
    _utf=$(printf '%s' "$_str" | tr -cd '▓' | wc -c | tr -d ' ')
    _utf_chars=$((_utf / 3))
    _viscols=$((_blen - _utf_chars * 2))
    _need=$((_want - _viscols))
    if [ "$_need" -gt 0 ]; then
        printf '%s' "$_str"
        printf "%${_need}s" ""
    else
        printf '%s' "$_str"
    fi
}

BW=68

# Status banner
printf '\n'
printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"

if [ "$total_failed" -eq 0 ] 2>/dev/null; then
    printf "  ${C}░░${R}  ${G}▓▓ ALL SUBSYSTEMS VERIFIED // 0 BREACHES DETECTED${R}            ${C}░░${R}\n"
else
    printf "  ${C}░░${R}  ${M}▓▓ INTEGRITY BREACH // %d FAILURES DETECTED${R}                  ${C}░░${R}\n" "$total_failed"
fi

printf "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}\n"

# Grand total box
printf '\n'
printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
printf "  ${M}║${R}  ${C}░▒▓${R} ${B}T E S T   M A T R I X   //   F I N A L   R E P O R T${R} ${C}▓▒░${R}  ${M}║${R}\n"
printf "  ${M}╠══════════════════════════════════════════════════════════════════════╣${R}\n"

scan_line=$(printf "[SCAN COMPLETE]  %d routines deployed across %d subsystems" "$total_tests" "$num_suites")
pass_line=$(printf "▓▓ PASSED .......... %d" "$total_passed")

if [ "$total_failed" -eq 0 ]; then
    fail_line="▓▓ FAILED .......... 0"
    stat_line=">> STATUS: ALL SUBSYSTEMS NOMINAL // 0 BREACHES DETECTED"
    fail_color="$G"
    stat_color="$G"
else
    fail_line=$(printf "▓▓ FAILED .......... %d" "$total_failed")
    stat_line=$(printf ">> WARNING: INTEGRITY BREACH // %d ANOMALIES DETECTED" "$total_failed")
    fail_color="$RED"
    stat_color="$RED"
fi

printf "  ${M}║${R}  ${DIM}$(pad "$scan_line" $BW)${R}${M}║${R}\n"
printf "  ${M}║${R}  $(pad "" $BW)${M}║${R}\n"
printf "  ${M}║${R}  ${G}$(pad "$pass_line" $BW)${R}${M}║${R}\n"
printf "  ${M}║${R}  ${fail_color}$(pad "$fail_line" $BW)${R}${M}║${R}\n"
printf "  ${M}║${R}  $(pad "" $BW)${M}║${R}\n"
printf "  ${M}║${R}  ${stat_color}$(pad "$stat_line" $BW)${R}${M}║${R}\n"
printf "  ${M}║${R}  $(pad "" $BW)${M}║${R}\n"
printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
printf '\n'

exit $any_failed
