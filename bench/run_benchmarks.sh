#!/bin/bash
# Run all iftopcolor benchmarks
# Usage: ./bench/run_benchmarks.sh [filter]
#   filter: optional substring to match benchmark names (e.g. "hash", "sorted", "vector", "pool")

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Build
echo "Building benchmarks..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
cmake --build "$BUILD_DIR" --target bench_hash bench_sorted_list bench_vector bench_pool -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) 2>&1 | tail -1

echo ""
echo "System: $(uname -ms)"
echo "CPU: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || cat /proc/cpuinfo 2>/dev/null | grep 'model name' | head -1 | cut -d: -f2)"
echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

FILTER="${1:-}"
BENCHMARKS="bench_hash bench_sorted_list bench_vector bench_pool"

for bench in $BENCHMARKS; do
    if [ -n "$FILTER" ] && [[ "$bench" != *"$FILTER"* ]]; then
        continue
    fi
    if [ -x "$BUILD_DIR/$bench" ]; then
        echo "================================================================"
        "$BUILD_DIR/$bench"
        echo ""
    fi
done

echo "All benchmarks complete."
