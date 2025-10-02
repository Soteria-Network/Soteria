#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

SOTERIAD=${SOTERIAD:-$SRCDIR/soteriad}
SOTERIACLI=${SOTERIACLI:-$SRCDIR/soteria-cli}
SOTERIAQT=${SOTERIAQT:-$SRCDIR/qt/soteria-qt}

[ ! -x $SOTERIAD ] && echo "$SOTERIAD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
SOTERVER=($($SOTERIACLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for soteriad if --version-string is not set,
# but has different outcomes for soteria-qt and soteria-cli.
echo "[COPYRIGHT]" > footer.h2m
$SOTERIAD --version | sed -n '1!p' >> footer.h2m

for cmd in $SOTERIAD $SOTERIACLI $SOTERIAQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${SOTERVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${SOTERVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
