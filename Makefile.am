#
# Makefile.am:
# Automake file for iftop.
#
# I don't understand this stuff, so I just stole it from the tpop3d
# distribution. That means that, really, Mark Longair should take credit for
# it.
#   -- Chris Lightfoot
#
# $Id: Makefile.am,v 1.11 2010/11/27 17:19:25 pdw Exp $
#

# PREFIX specifies the base directory for the installation.
PREFIX = /usr/local
# BINDIR is where the binary lives relative to PREFIX (no leading /).
BINDIR = sbin
MANDIR = share/man

AUTOMAKE_OPTIONS = gnu

SUBDIRS = src

install: iftop
	install -D src/iftop   $(PREFIX)/$(BINDIR)/iftop
	install -D man/iftop.8 $(PREFIX)/$(MANDIR)/man8/iftop.8

uninstall:
	rm -f $(PREFIX)/$(BINDIR)/iftop $(PREFIX)/$(MANDIR)/man8/iftop.8

clean:
	rm -f *~ src/*.o core src/iftop iftop.spec
