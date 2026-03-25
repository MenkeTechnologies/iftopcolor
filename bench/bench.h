/*
 * bench.h: lightweight benchmarking harness
 */

#ifndef BENCH_H
#define BENCH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

/* High-resolution timer */
static inline uint64_t bench_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* Prevent dead-code elimination */
static volatile int bench_sink;
static inline void bench_use(int x) { bench_sink = x; }

#define BENCH_RUN(name, iterations, body) do { \
    uint64_t _iters = (iterations); \
    /* warmup */ \
    for (uint64_t _i = 0; _i < (_iters / 10 > 0 ? _iters / 10 : 1); _i++) { body; } \
    uint64_t _t0 = bench_now_ns(); \
    for (uint64_t _i = 0; _i < _iters; _i++) { body; } \
    uint64_t _t1 = bench_now_ns(); \
    double _total_ms = (double)(_t1 - _t0) / 1e6; \
    double _per_op_ns = (double)(_t1 - _t0) / (double)_iters; \
    double _ops_sec = _iters / (_total_ms / 1000.0); \
    printf("  %-45s %10.2f ms  %8.1f ns/op  %12.0f ops/s\n", \
           (name), _total_ms, _per_op_ns, _ops_sec); \
} while(0)

#define BENCH_SECTION(name) printf("\n=== %s ===\n", (name))

#endif /* BENCH_H */
