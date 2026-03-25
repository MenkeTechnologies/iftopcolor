```
     ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
     ██╗███████╗████████╗ ██████╗ ██████╗  ██████╗ ██████╗ ██╗      ██████╗ ██████╗
     ██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝██╔═══██╗██║     ██╔═══██╗██╔══██╗
     ██║█████╗     ██║   ██║   ██║██████╔╝██║     ██║   ██║██║     ██║   ██║██████╔╝
     ██║██╔══╝     ██║   ██║   ██║██╔═══╝ ██║     ██║   ██║██║     ██║   ██║██╔══██╗
     ██║██║        ██║   ╚██████╔╝██║     ╚██████╗╚██████╔╝███████╗╚██████╔╝██║  ██║
     ╚═╝╚═╝        ╚═╝    ╚═════╝ ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═╝
     ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
```

<p align="center">
  <strong>[ REAL-TIME BANDWIDTH SURVEILLANCE // FULL SPECTRUM COLOR ]</strong>
</p>

<p align="center">
  <code>>>> JACK IN. WATCH THE DATA FLOW. OWN YOUR NETWORK. <<<</code>
</p>

```
 ┌──────────────────────────────────────────────────────────────────────────┐
 │  STATUS: ONLINE          THREAT LEVEL: NEON          SIGNAL: ████████░░ │
 └──────────────────────────────────────────────────────────────────────────┘
```

---

> _"The street finds its own uses for things."_ -- William Gibson
>
> _A colorized fork of [iftop](http://www.ex-parrot.com/pdw/iftop/) that renders real-time bandwidth usage on a network interface, broken down by host pairs. Fully customizable color schemes via config file. Your console, your rules._

![screenshot of iftopcolor](/screenshot1.png)

---

## `[0x01]` SYSTEM REQUIREMENTS

```
SCANNING DEPENDENCIES...
PROBING HARDWARE INTERFACES...
ENUMERATING ATTACK SURFACE...
```

| Component | Status |
|---|---|
| CMake 3.10+ | `REQUIRED` |
| libpcap | `REQUIRED` -- packet capture substrate |
| ncurses (ncursesw on Linux) | `REQUIRED` -- terminal rendering engine |
| pthreads | `REQUIRED` -- parallel execution threads |

### Platform acquisition vectors

| Platform | Command |
|---|---|
| Debian/Ubuntu | `sudo apt install cmake libpcap-dev libncursesw5-dev` |
| Fedora/RHEL | `sudo dnf install cmake libpcap-devel ncurses-devel` |
| macOS | `brew install cmake` (libpcap and ncurses ship with Xcode CLI tools) |
| FreeBSD | `pkg install cmake` |

---

## `[0x02]` INSTALLATION

```
INITIATING BUILD SEQUENCE...
DECRYPTING SOURCE ARCHIVE...
COMPILING INTRUSION COUNTERMEASURES...
```

```sh
# --- DOWNLOAD THE PAYLOAD ---
git clone https://github.com/MenkeTechnologies/iftopcolor.git
cd iftopcolor

# --- COMPILE THE ICE ---
mkdir build && cd build && cmake ..
make

# --- DEPLOY TO /usr/local/sbin ---
sudo make install
```

> Make sure `/usr/local/sbin` is in your `PATH` or you'll be flatlined at the prompt.

---

## `[0x03]` USAGE

```
INITIALIZING PACKET SNIFFER...
ELEVATING PRIVILEGES...
ENTERING THE MATRIX...
```

iftop requires root privileges to capture packets:

```sh
sudo iftop
```

### Command-line switches

```
 SWITCH              FUNCTION
 ──────              ────────
 -h                  display help
 -n                  don't do hostname lookups
 -N                  don't convert port numbers to services
 -p                  run in promiscuous mode
 -b                  don't display bar graph of traffic
 -B                  display bandwidth in bytes
 -i interface        listen on named interface
 -f filter code      BPF filter for packet selection
 -F net/mask         show traffic in/out of IPv4 network
 -G net6/mask6       show traffic in/out of IPv6 network
 -l                  display and count link-local IPv6 traffic
 -P                  show ports as well as hosts
 -m limit            set upper limit for bandwidth scale
 -c config file      use alternative configuration file
```

### Interactive keymap

```
 ┌─────────────────────────────────┬─────────────────────────────────┐
 │ KEY   ACTION                    │ KEY   ACTION                    │
 │ ───   ──────                    │ ───   ──────                    │
 │  n    Toggle DNS resolution     │  P    Pause display             │
 │  s    Toggle source host        │  h    Toggle help               │
 │  d    Toggle destination host   │  b    Toggle bar graph          │
 │  t    Cycle line display mode   │  B    Cycle bar graph average   │
 │  N    Toggle service resolution │  T    Toggle cumulative totals  │
 │  S    Toggle source port        │ j/k   Scroll display            │
 │  D    Toggle destination port   │  f    Edit filter code          │
 │  p    Toggle port display       │  l    Set screen filter         │
 │ 1/2/3 Sort by column            │  L    Lin/log scales            │
 │ < / > Sort by src/dst name      │  !    Shell command             │
 │  o    Freeze current order      │  q    Quit                      │
 └─────────────────────────────────┴─────────────────────────────────┘
```

---

## `[0x04]` COLOR CONFIGURATION

> _"The sky above the port was the color of television, tuned to a dead channel."_
> _Your terminal doesn't have to be._

```
LOADING CHROMATIC SUBSYSTEM...
PARSING COLOR MATRIX...
APPLYING NEON OVERLAY...
```

Create `~/.iftopcolors` to customize colors. Available colors: `red`, `blue`, `green`, `yellow`, `magenta`, `cyan`, `black`, `white`. Each can be `bold` or `nonbold`.

```ini
# ~/.iftopcolors -- NEON NIGHTCITY PRESET
BOTH_BAR_COLOR              blue    bold
HOST1_COLOR                 red     nonbold
HOST2_COLOR                 red     bold
SCALE_BAR_COLOR             red     nonbold
SCALE_MARKERS_COLOR         red     bold
DL_UL_INDICATOR_COLOR       blue    bold
TWO_SECOND_TRANSFER_COLUMN_COLOR    yellow  bold
TEN_SECOND_TRANSFER_COLUMN_COLOR    magenta bold
FOURTY_SECOND_TRANSFER_COLUMN_COLOR yellow  bold
BOTTOM_BAR_COLOR            blue    bold
CUM_LABEL_COLOR             yellow  nonbold
PEAK_LABEL_COLOR            magenta bold
RATES_LABEL_COLOR           magenta bold
TOTAL_LABEL_COLOR           blue    bold
CUM_TRANSFER_COLUMN_COLOR   magenta nonbold
PEAK_TRANSFER_COLUMN_COLOR  magenta nonbold
RECEIVE_BAR_COLOR           yellow  nonbold
SENT_BAR_COLOR              yellow  nonbold
```

---

## `[0x05]` TESTS

```
DEPLOYING DIAGNOSTIC ROUTINES...
VALIDATING MEMORY SUBSYSTEMS...
INTEGRITY CHECK IN PROGRESS...
```

A test suite covers core data structures and utilities: hash tables, address/namespace/service hashes, sorted lists, vectors, string maps, config file parsing, and utility functions.

### Running tests

```sh
mkdir build && cd build && cmake ..
make check
```

Run a single diagnostic:

```sh
make check_hash
./check_hash
```

### Test matrix

```
 ┌───────────────────────┬──────────────────────────────────────────────────────────────┬───────┐
 │ SUITE                 │ SUBSYSTEM COVERAGE                                           │ TESTS │
 ├───────────────────────┼──────────────────────────────────────────────────────────────┼───────┤
 │ check_util            │ xmalloc, xcalloc, xrealloc, xstrdup, xfree, alloc chains   │    41 │
 │ check_vector          │ Push, pop, remove, resize, iteration, shrink, ptr storage   │    68 │
 │ check_hash            │ Generic hash: insert, find, delete, collisions, high load   │    59 │
 │ check_addr_hash       │ IPv4/IPv6 pairs, pool allocator, fast-path, UDP, protocol   │    49 │
 │ check_ns_hash         │ IPv6 address to hostname cache, adjacent addrs, scaling     │    31 │
 │ check_serv_hash       │ Port+protocol to service name, protocol differentiation     │    30 │
 │ check_sorted_list     │ Single insert, batch insert, descending order, destroy      │    46 │
 │ check_stringmap       │ Binary tree insert, find, special chars, deep chains        │    46 │
 │ check_cfgfile         │ Config parsing: string, bool, int, float, enum, file IO     │    77 │
 │ check_leaks           │ Full lifecycle leak tests for all data structures (macOS)   │    30 │
 └───────────────────────┴──────────────────────────────────────────────────────────────┴───────┘
                                                                                TOTAL:    477
```

### Memory leak analysis

Leak detection uses macOS `leaks --atExit` to verify zero unreachable allocations after full create/populate/destroy lifecycles for every data structure.

```sh
make leakcheck
```

Covers: vector, generic hash, address hash (pool allocator), ns_hash, serv_hash, sorted list (single and batch), stringmap, and config file subsystems.

---

## `[0x06]` BENCHMARKS

```
INITIALIZING PERFORMANCE PROFILER...
WARMING UP EXECUTION CORES...
MEASURING CLOCK CYCLES...
ALL GOVERNORS SET TO MAXIMUM OVERDRIVE...
```

> _"Speed is the ultimate weapon in the Net."_

Compiled with aggressive optimizations (`-O3`, `-march=native`, `-flto`, `-funroll-loops`).

### Running benchmarks

```sh
# --- FULL BENCHMARK SWEEP ---
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
make bench

# --- TARGETED STRIKE ---
./bench/run_benchmarks.sh hash
./bench/run_benchmarks.sh vector
```

### Benchmark suites

```
 ┌────────────────────────────┬───────────────────────────────────────────────────────────────────────┐
 │ SUITE                      │ MEASURES                                                              │
 ├────────────────────────────┼───────────────────────────────────────────────────────────────────────┤
 │ benchmark_hash             │ Distribution, insert, find (hit/miss), scaling, delete, iterate       │
 │ benchmark_sorted_list      │ Single O(n^2) vs batch O(n log n), iteration scaling, destroy, cycles │
 │ benchmark_vector           │ Push, pop, remove (front/middle/back), iterate, reallocate, pointers  │
 │ benchmark_pool             │ Pool vs malloc: insert/delete, churn, multi-block, reuse, post-churn  │
 │ benchmark_stringmap        │ Sequential vs random keys, find scaling, config-style workload        │
 │ benchmark_ns_hash          │ IPv6 address cache: insert, find, delete, iteration, churn            │
 │ benchmark_serv_hash        │ Service cache: insert, find, delete, iteration, TCP/UDP diff          │
 │ benchmark_cfgfile          │ Config set/get, type parsing, file reading, realistic workload        │
 └────────────────────────────┴───────────────────────────────────────────────────────────────────────┘
```

### Results -- performance telemetry

```
DOWNLOADING BENCHMARK DATA FROM CORES...
RENDERING PERFORMANCE HEATMAP...
```

#### `addr_hash` vs `hash` -- pool-allocated ICE vs generic malloc

> _The pool allocator is a custom slab that recycles fixed-size nodes. No syscalls. No fragmentation. Pure speed._

```
 OPERATION                     addr_hash [OPTIMIZED]     hash [GENERIC]        SPEEDUP
 ─────────                     ─────────────────────     ──────────────        ───────
 Insert 1000 IPv4 flows        10,710 ns/op              23,110 ns/op          2.2x
 Find hit (1000 entries)            2.5 ns/op                  4.0 ns/op       1.6x
```

```
 ALLOCATOR SHOOTOUT            TIME          VERDICT
 ──────────────────            ────          ───────
 Pool  (insert 2000 + delete)  24.7 ms       >>> 2.1x FASTER <<<
 Malloc (insert 2000 + delete) 51.6 ms       baseline
```

Hash distribution is near-ideal -- IPv4 stddev 2.26 vs ideal 2.21, IPv6 stddev 2.21 vs ideal 2.21. Virtually zero wasted buckets.

#### `ns_hash` vs `serv_hash` -- specialized hash variant duel

> _Two hashes. Same generic backbone. Wildly different key structures. The bottleneck is always comparison._

```
 VARIANT        FIND HIT (200 entries)     THROUGHPUT          KEY COMPARISON
 ───────        ──────────────────────     ──────────          ──────────────
 ns_hash              2.6 ns/op            384M ops/s          IN6_ARE_ADDR_EQUAL (16-byte memcmp)
 serv_hash           53.4 ns/op           18.7M ops/s          compound struct (port + protocol)
                                                                ~~~ 20x DELTA ~~~
```

#### Sorted list -- brute force O(n^2) vs batch O(n log n)

> _At small n the overhead of qsort dominates. Cross the threshold and batch mode pulls away exponentially._

```
 SIZE      SINGLE (one-by-one)     BATCH (qsort)       SPEEDUP
 ────      ───────────────────     ─────────────       ───────
   100          4,290 ns/op          6,445 ns/op        0.7x [batch overhead]
   500         73,450 ns/op         43,656 ns/op        1.7x
  1000        230,920 ns/op         98,820 ns/op        2.3x
  2000        766,900 ns/op        205,380 ns/op        3.7x
  5000      4,585,000 ns/op        613,951 ns/op   >>> 7.5x <<<
```

#### Stringmap -- unbalanced BST under duress

> _Sequential keys are a death sentence for an unbalanced binary tree. The structure degenerates into a linked list. Every lookup walks the full chain._

```
 OPERATION (1000 keys)     SEQUENTIAL [DEGENERATE]     RANDOM [BALANCED-ISH]     PENALTY
 ─────────────────────     ───────────────────────     ─────────────────────     ───────
 Insert                         873,510 ns/op              66,974 ns/op           13x
 Find hit                         758.8 ns/op                56.5 ns/op          13.4x
```

```
 VERDICT: Sequential keys => O(n) per op.  Random keys => O(log n).
          Choose your key distribution wisely, choomba.
```

#### Vector -- contiguous memory ops

> _Flat arrays. No pointers to chase. The CPU cache is your ally._

```
 OPERATION                          PERFORMANCE
 ─────────                          ───────────
 push_back 10,000 items             4,435 ns/op         amortized O(1)
 remove from front (1000 items)    36,240 ns/op         O(n) memmove
 iterate 10,000 items                574.6 ns/pass      ~0.057 ns/element
```

```
 NOTE: Iteration at cache-line speed. 0.057 ns/element = ~17.5 billion elements/sec.
       The bottleneck is your RAM, not the code.
```

---

## `[0x07]` KNOWN GLITCHES IN THE MATRIX

```
WARNING: THE FOLLOWING ANOMALIES HAVE BEEN DETECTED IN THE SYSTEM...
```

**`RedHat 7.2`** -- A bug in the bundled ncurses can cause a segfault. Update ncurses from Rawhide.

**`Slackware 8.1`** -- You may need to upgrade libpcap (via the tcpdump package) to compile.

**`FreeBSD 4.7`** -- Lacks a proper `gethostbyaddr_r`. Use `--with-resolver=...` to select an alternative resolver.

**`Solaris`** -- iftop must run in promiscuous mode to capture outgoing packets. This is auto-configured; the `-p` flag disables the non-broadcast packet filter. You may also need ncurses instead of the bundled curses.

---

## `[0xFF]` LICENSE

```
 ┌──────────────────────────────────────────────────────────────┐
 │  Copyright (c) 2026 Jacob Menke and contributors.           │
 │  UNAUTHORIZED REPRODUCTION WILL BE MET WITH FULL ICE.       │
 └──────────────────────────────────────────────────────────────┘
```

---

<p align="center">
<code>
 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
 ░░  CONNECTION TERMINATED // END OF LINE  ░░
 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
</code>
</p>
