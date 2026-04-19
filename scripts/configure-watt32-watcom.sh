#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
HOST_HOME=${HOME:-/home/$USER}
WATT_ROOT=${WATT_ROOT:-"$ROOT_DIR/3rd/watt32/src"}
SRC_DIR="$WATT_ROOT/src"
UTIL_DIR="$WATT_ROOT/util"
PREP="$ROOT_DIR/scripts/watt32-prep-make.py"
GEN_SYSERR="$ROOT_DIR/scripts/watt32-gen-syserr.py"

cd "$SRC_DIR"

# make -C "$UTIL_DIR" linux

sed -i 's/^#undef USE_BSD_API/#define USE_BSD_API/' config.h
sed -i 's/^#undef USE_BUFFERED_IO/#define USE_BUFFERED_IO/' config.h
sed -i 's/#define MAX_PORT  5000/#define MAX_PORT  USHRT_MAX/' ports.c

mkdir -p build/watcom/small build/watcom/large build/watcom/flat build/watcom/win32

python3 "$PREP" makefile.all watcom_s.mak WATCOM SMALL
python3 "$PREP" makefile.all watcom_l.mak WATCOM LARGE
python3 "$PREP" makefile.all watcom_f.mak WATCOM FLAT
python3 "$PREP" makefile.all watcom_w.mak WATCOM WIN32

../util/linux/mkdep -s.obj '-p$(OBJDIR)/' *.[ch] > build/watcom/watt32.dep
echo 'neterr.c: build/watcom/syserr.c' >> build/watcom/watt32.dep
python3 "$GEN_SYSERR" ../inc/sys/watcom.err build/watcom/syserr.c

echo "Generated Watcom makefiles in $SRC_DIR"
echo "Next: wmake -f watcom_l.mak"
