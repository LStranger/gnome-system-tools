#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="the package."

(test -f $srcdir/configure.in \
 && test -f $srcdir/src/dhcpd/dhcpd.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level directory"
    echo ""
    echo "Perhaps you need to \"cvs -z3 co ximian-setup-tools\" in \"..\" ?"
    echo
    exit 1
}

. $srcdir/macros/autogen.sh
