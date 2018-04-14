# MenkeTechnologies Iftop **_FULL_** COLOR

![screenshot of all colorized iftop](/screenshot1.png)


# Installation

```sh
git clone https://github.com/MenkeTechnologies/iftopcolor.git && cd iftopcolor && ./configure && make && sudo make install
```

## Should be installed to /usr/local/sbin.  So make sure this in PATH.

```sh
sudo iftop
```

# Configuration File at ~/.iftocolors.  Will be used for custom colors if present and parseable.
```
#colors are red, blue, green, yellow, magenta, cyan, black and white
#RECEIVE_BAR_COLOR yellow -
#SENT_BAR_COLOR yellow -
BOTH_BAR_COLOR blue bold
#SCALE_BAR_COLOR red nonbold
#SCALE_MARKERS_COLOR red bold
#DL_UL_INDICATOR_COLOR blue bold
HOST1_COLOR red nonbold
#HOST2_COLOR red bold
#TWO_SECOND_TRANSFER_COLUMN_COLOR yellow bold
#TEN_SECOND_TRANSFER_COLUMN_COLOR magenta bold
#FOURTY_SECOND_TRANSFER_COLUMN_COLOR yellow bold
#BOTTOM_BAR_COLOR blue bold
#CUM_LABEL_COLOR yellow nonbold
#PEAK_LABEL_COLOR magenta bold
#RATES_LABEL_COLOR magenta bold
#TOTAL_LABEL_COLOR blue bold
#CUM_TRANSFER_COLUMN_COLOR magenta nonbold
#PEAK_TRANSFER_COLUMN_COLORt magenta nonbold
```
iftop must be run as root.

# KNOWN ISSUES

# RedHat 7.2:

There is a bug in the version of ncurses distibuted with RedHat 7.2 that
will cause iftop to segfault.  The RPM in RedHat's Rawhide distribution
fixes this.

# Slackware 8.1:

You may need to upgrade your libpcap (by updating the tcpdump package) 
in order to compile iftop.

# FreeBSD 4.7:

This version of FreeBSD lacks a proper gethostbyaddr_r function. You should
choose an alternative name resolution technique using the --with-resolver=...
option to configure.

# Solaris:

On Solaris, iftop has to run in promiscuous mode in order to capture
outgoing packets.  iftop autoconfigures to run in promiscuous mode on
Solaris, but will filter out non-broadcast packets which are not
addressed to or from localhost.  On Solaris, the -p option merely
disables that filter.

If you have some other sort of system that behaves like Solaris in
needing promiscuous mode, you can pass --enable-default-promiscuous to
configure to enable this behavior.

Cf. http://www.tcpdump.org/lists/workers/2002/02/msg00010.html

The version of curses distributed with Solaris may not be sufficient for
iftop's needs. You will probably need ncurses or similar.

