```
                 ██╗███████╗████████╗ ██████╗ ██████╗  ██████╗ ██████╗ ██╗      ██████╗ ██████╗
                 ██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝██╔═══██╗██║     ██╔═══██╗██╔══██╗
                 ██║█████╗     ██║   ██║   ██║██████╔╝██║     ██║   ██║██║     ██║   ██║██████╔╝
                 ██║██╔══╝     ██║   ██║   ██║██╔═══╝ ██║     ██║   ██║██║     ██║   ██║██╔══██╗
                 ██║██║        ██║   ╚██████╔╝██║     ╚██████╗╚██████╔╝███████╗╚██████╔╝██║  ██║
                 ╚═╝╚═╝        ╚═╝    ╚═════╝ ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═╝
```

<p align="center">
  <strong>[ REAL-TIME BANDWIDTH SURVEILLANCE // FULL SPECTRUM COLOR ]</strong>
</p>

<p align="center">
  <code>>>> JACK IN. WATCH THE DATA FLOW. OWN YOUR NETWORK. <<<</code>
</p>

---

> _A colorized fork of [iftop](http://www.ex-parrot.com/pdw/iftop/) that renders real-time bandwidth usage on a network interface, broken down by host pairs. Fully customizable color schemes via config file. Your console, your rules._

![screenshot of iftopcolor](/screenshot1.png)

---

## `[0x01]` SYSTEM REQUIREMENTS

```
SCANNING DEPENDENCIES...
```

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

---

## `[0x02]` INSTALLATION

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

```
 KEY   ACTION                    KEY   ACTION
 ───   ──────                    ───   ──────
  n    Toggle DNS resolution      P    Pause display
  s    Toggle source host         h    Toggle help
  d    Toggle destination host    b    Toggle bar graph
  t    Cycle line display mode    B    Cycle bar graph average
  N    Toggle service resolution  T    Toggle cumulative totals
  S    Toggle source port        j/k   Scroll display
  D    Toggle destination port    f    Edit filter code
  p    Toggle port display        l    Set screen filter
 1/2/3 Sort by column             L    Lin/log scales
 < / > Sort by src/dst name       !    Shell command
  o    Freeze current order       q    Quit
```

---

## `[0x04]` COLOR CONFIGURATION

> _"The sky above the port was the color of television, tuned to a dead channel."_
> _Your terminal doesn't have to be._

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

A test suite covers core data structures and utilities: hash tables, address/namespace/service hashes, sorted lists, vectors, string maps, config file parsing, and utility functions.

### Running tests

```sh
mkdir build && cd build && cmake ..
make check
```

Run a single test:

```sh
make test_hash
./test_hash
```

### Test suites

| Suite | Covers |
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

---

## `[0x06]` BENCHMARKS

A benchmark suite measures the performance of core data structures: hash tables, sorted lists, vectors, and the pool allocator.

### Running benchmarks

```sh
# Build and run all benchmarks
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
make bench

# Run a specific benchmark suite
./bench/run_benchmarks.sh hash
./bench/run_benchmarks.sh vector
```

> Compiled with aggressive optimizations (`-O3`, `-march=native`, `-flto`, `-funroll-loops`).

### What is benchmarked

| Suite | Measures |
|---|---|
| `bench_hash` | Hash distribution, insert, find (hit/miss), find scaling, delete, iteration, mixed workload |
| `bench_sorted_list` | Single insert O(n^2) vs batch insert O(n log n), iteration |
| `bench_vector` | Push back, push/pop, remove, iteration at various sizes |
| `bench_pool` | Pool allocator vs malloc/free for insert/delete and churn workloads |

---

## `[0x07]` KNOWN GLITCHES IN THE MATRIX

**RedHat 7.2** -- A bug in the bundled ncurses can cause a segfault. Update ncurses from Rawhide.

**Slackware 8.1** -- You may need to upgrade libpcap (via the tcpdump package) to compile.

**FreeBSD 4.7** -- Lacks a proper `gethostbyaddr_r`. Use `--with-resolver=...` to select an alternative resolver.

**Solaris** -- iftop must run in promiscuous mode to capture outgoing packets. This is auto-configured; the `-p` flag disables the non-broadcast packet filter. You may also need ncurses instead of the bundled curses.

---

## `[0xFF]` LICENSE

```
Copyright (c) 2026 Jacob Menke and contributors.
```

---

<p align="center"><code>// END OF LINE //</code></p>
