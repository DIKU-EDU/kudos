## Makefile for the toplevel directory of YAMS.
##
## Copyright (C) 2002-2006 Juha Aatrokoski, Timo Lilja, Leena Salmela,
##   Teemu Takanen, Aleksi Virtanen
##
## Copyright (C) 1997-98 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in

#AUTOMAKE_OPTIONS = 1.5 gnits dist-bzip2
AUTOMAKE_OPTIONS = 1.7 foreign

ACLOCAL_AMFLAGS = -I m4

MAINT_CHARSET = latin1

# previous version
ke_PRE = @ke_PRE@

EXTRA_DIST = BUGS CHANGES src/yams.conf makeintformat.c semtest.c

SUBDIRS = src man info

# To produce a diff against the previous version, untar this version
# and the previous version in the same directory and run `make diff'.
# Only the maintainer should do this.
diffcheck:
	for d in $(PACKAGE)-$(ke_PRE) $(PACKAGE)-$(VERSION) ; do \
	    if test ! -d $$d ; then \
	        if test -r $$d.tar.gz ; then \
	            tar -zxf $$d.tar.gz ; \
                else \
	            echo subdir $$d does not exist. ; \
	            exit 1 ; \
                fi ; \
	    fi ; \
	done

diff: diffcheck
	echo -e "\
To apply these patches, cd to the main directory of the package\n\
and then use \`patch -p1 < yams-XXX.diff'.\n\
Before building the programm run \`autoconf' and \`autoheader' to rebuild\n\
\`configure' and \`config.h.in'.\n\n" > \
	    $(PACKAGE)-$(ke_PRE)-$(VERSION).diff
	-diff -rc2P --exclude=configure --exclude=config.h.in --exclude=*.info \
	    --exclude=*.gmo --exclude=aclocal.m4 \
	    $(PACKAGE)-$(ke_PRE) $(PACKAGE)-$(VERSION) >> \
	    $(PACKAGE)-$(ke_PRE)-$(VERSION).diff
	gzip --force --best $(PACKAGE)-$(ke_PRE)-$(VERSION).diff


## Makefile.am ends here
