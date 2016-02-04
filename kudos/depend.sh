#!/bin/sh
#
# A script to calculate the header file dependencies
#
# Taken from the paper  _Recursive Makefile Considered Harmful_
# by Peter Miller
#
# $Id: depend.sh,v 1.4 2003/02/11 14:16:28 tlilja Exp $
set -e

DIR="$1"
shift 1

case "$DIR" in
    "" | ".")
    $CC -MM -MG "$@" | sed -e 's@^\(.*\).o:@\1.d \1.o:@'
    ;;
    *)
    $CC -MM -MG "$@" | sed -e "s@^\(.*\)\.o:@$DIR/\1.o:@"
    ;;
esac

