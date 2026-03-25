#!/bin/bash
# Run all iftopcolor benchmarks
# Usage: ./bench/run_benchmarks.sh [filter]
#   filter: optional substring to match benchmark names (e.g. "hash", "sorted", "vector", "pool")

set -e

# ANSI codes
C="\033[0;36;1m"  # bold cyan
G="\033[0;32;1m"  # bold green
M="\033[0;35;1m"  # bold magenta
B="\033[1m"       # bold
D="\033[2m"       # dim
R="\033[0m"       # reset

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo ""
echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo -e "  ${C}░░${R}  ${B}COMPILING BENCHMARK ICE...${R}                                        ${C}░░${R}"
echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo ""

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
cmake --build "$BUILD_DIR" --target benchmark_hash benchmark_sorted_list benchmark_vector benchmark_pool benchmark_stringmap benchmark_ns_hash benchmark_serv_hash benchmark_cfgfile -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) 2>&1 | tail -1

SYS="$(uname -ms)"
CPU="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || cat /proc/cpuinfo 2>/dev/null | grep 'model name' | head -1 | cut -d: -f2)"
DATE="$(date '+%Y-%m-%d %H:%M:%S')"

echo ""
echo -e "  ${D}┌──────────────────────────────────────────────────────────────────────┐${R}"
echo -e "  ${D}│${R}  ${M}SYSTEM${R}  $SYS"
echo -e "  ${D}│${R}  ${M}CPU${R}     $CPU"
echo -e "  ${D}│${R}  ${M}DATE${R}    $DATE"
echo -e "  ${D}└──────────────────────────────────────────────────────────────────────┘${R}"
echo ""

FILTER="${1:-}"
BENCHMARKS="benchmark_hash benchmark_sorted_list benchmark_vector benchmark_pool benchmark_stringmap benchmark_ns_hash benchmark_serv_hash benchmark_cfgfile"

for bench in $BENCHMARKS; do
    if [ -n "$FILTER" ] && [[ "$bench" != *"$FILTER"* ]]; then
        continue
    fi
    if [ -x "$BUILD_DIR/$bench" ]; then
        "$BUILD_DIR/$bench"
        echo ""
    fi
done

echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo -e "  ${C}░░${R}  ${G}▓▓ ALL BENCHMARKS COMPLETE // CONNECTION TERMINATED${R}          ${C}░░${R}"
echo -e "  ${C}░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░${R}"
echo ""
