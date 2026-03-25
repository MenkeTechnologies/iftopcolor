# iftopcolor

A colorized fork of [iftop](http://www.ex-parrot.com/pdw/iftop/) that displays real-time bandwidth usage on a network interface, broken down by host pairs. Fully customizable color schemes via a simple config file.

![screenshot of iftopcolor](/screenshot1.png)

## Requirements

- CMake 3.10+
- libpcap
- ncurses (ncursesw on Linux)
- pthreads

### Platform packages

| Platform | Command |
|---|---|
| Debian/Ubuntu | `sudo apt install cmake libpcap-dev libncursesw5-dev` |
| Fedora/RHEL | `sudo dnf install cmake libpcap-devel ncurses-devel` |
| macOS | `brew install cmake` (libpcap and ncurses ship with Xcode CLI tools) |
| FreeBSD | `pkg install cmake` |

## Installation

```sh
git clone https://github.com/MenkeTechnologies/iftopcolor.git
cd iftopcolor
cmake -B build
cmake --build build
sudo cmake --install build
```

The binary installs to `/usr/local/sbin`. Make sure this directory is in your `PATH`.

## Usage

iftop requires root privileges to capture packets:

```sh
sudo iftop
```

### Command-line options

```
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

### Interactive keys

| Key | Action | | Key | Action |
|-----|--------|-|-----|--------|
| `n` | Toggle DNS resolution | | `P` | Pause display |
| `s` | Toggle source host | | `h` | Toggle help |
| `d` | Toggle destination host | | `b` | Toggle bar graph |
| `t` | Cycle line display mode | | `B` | Cycle bar graph average |
| `N` | Toggle service resolution | | `T` | Toggle cumulative totals |
| `S` | Toggle source port | | `j/k` | Scroll display |
| `D` | Toggle destination port | | `f` | Edit filter code |
| `p` | Toggle port display | | `l` | Set screen filter |
| `1/2/3` | Sort by column | | `L` | Lin/log scales |
| `<` / `>` | Sort by source/dest name | | `!` | Shell command |
| `o` | Freeze current order | | `q` | Quit |

## Color configuration

Create `~/.iftopcolors` to customize colors. Available colors: `red`, `blue`, `green`, `yellow`, `magenta`, `cyan`, `black`, `white`. Each color can be `bold` or `nonbold`.

```
# Example ~/.iftopcolors
BOTH_BAR_COLOR blue bold
HOST1_COLOR red nonbold
HOST2_COLOR red bold
SCALE_BAR_COLOR red nonbold
SCALE_MARKERS_COLOR red bold
DL_UL_INDICATOR_COLOR blue bold
TWO_SECOND_TRANSFER_COLUMN_COLOR yellow bold
TEN_SECOND_TRANSFER_COLUMN_COLOR magenta bold
FOURTY_SECOND_TRANSFER_COLUMN_COLOR yellow bold
BOTTOM_BAR_COLOR blue bold
CUM_LABEL_COLOR yellow nonbold
PEAK_LABEL_COLOR magenta bold
RATES_LABEL_COLOR magenta bold
TOTAL_LABEL_COLOR blue bold
CUM_TRANSFER_COLUMN_COLOR magenta nonbold
PEAK_TRANSFER_COLUMN_COLOR magenta nonbold
RECEIVE_BAR_COLOR yellow nonbold
SENT_BAR_COLOR yellow nonbold
```

## Tests

A test suite covers core data structures and utilities: hash tables, address/namespace/service hashes, sorted lists, vectors, string maps, config file parsing, and utility functions.

### Running tests

```sh
cmake -B build
cmake --build build --target test
```

This builds and runs all test binaries. You can also run a single test:

```sh
cmake --build build --target test_hash
./build/test_hash
```

### Test suites

| Suite | What it covers |
|---|---|
| `test_util` | Utility functions |
| `test_vector` | Dynamic vector operations |
| `test_hash` | Core hash table |
| `test_addr_hash` | Address-keyed hash |
| `test_ns_hash` | Namespace hash |
| `test_serv_hash` | Service hash |
| `test_sorted_list` | Sorted list insertion and iteration |
| `test_stringmap` | String map lookups |
| `test_cfgfile` | Configuration file parsing |

## Benchmarks

A benchmark suite is included to measure the performance of core data structures: hash tables, sorted lists, vectors, and the pool allocator.

### Running benchmarks

```sh
# Build and run all benchmarks
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target bench_hash bench_sorted_list bench_vector bench_pool
./bench/run_benchmarks.sh

# Run a specific benchmark suite
./bench/run_benchmarks.sh hash
./bench/run_benchmarks.sh vector
```

Benchmarks are compiled with aggressive optimizations (`-O3`, `-march=native`, `-flto`, `-funroll-loops`).

### What is benchmarked

| Suite | What it measures |
|---|---|
| `bench_hash` | Hash distribution, insert, find (hit/miss), find scaling, delete, iteration, mixed workload |
| `bench_sorted_list` | Single insert O(n^2) vs batch insert O(n log n), iteration |
| `bench_vector` | Push back, push/pop, remove, iteration at various sizes |
| `bench_pool` | Pool allocator vs malloc/free for insert/delete and churn workloads |

## Known issues

**RedHat 7.2** -- A bug in the bundled ncurses can cause a segfault. Update ncurses from Rawhide.

**Slackware 8.1** -- You may need to upgrade libpcap (via the tcpdump package) to compile.

**FreeBSD 4.7** -- Lacks a proper `gethostbyaddr_r`. Use `--with-resolver=...` to select an alternative resolver.

**Solaris** -- iftop must run in promiscuous mode to capture outgoing packets. This is auto-configured; the `-p` flag disables the non-broadcast packet filter. You may also need ncurses instead of the bundled curses.

## License

Copyright (c) 2002 Paul Warren and contributors.
