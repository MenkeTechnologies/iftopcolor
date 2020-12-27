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


SUBDIRS = src config

man_MANS = man/iftop.8

iftop.cat: iftop.8
	(echo -e ".pl 1100i" ; cat iftop.8 ; echo ".pl \n(nlu+10") | groff -Tascii -man > iftop.cat

iftop.spec: iftop.spec.in configure.in
	sed 's/__VERSION__/$(VERSION)/' < iftop.spec.in > iftop.spec

## These need to be distributed along with configure:

EXTRA_DIST = bootstrap README ChangeLog AUTHORS COPYING INSTALL TODO \
		Makefile.OLD $(man_MANS) iftop.cat iftop.spec
             
MAINTERCLEANFILES = Makefile.in aclocal.m4 configure configuration.h.in \
                    stamp-h.in iftop.spec

ACLOCAL = aclocal -I @ac_aux_dir@

