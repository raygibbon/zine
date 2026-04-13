#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD="$ROOT/build/zine"
HOST_HOME=${HOME:-/home/$USER}
WATCOM_ROOT=${WATCOM:-"$HOST_HOME/opt/watcom"}

mkdir -p "$BUILD"
mkdir -p "$ROOT/mnt"
cd "$BUILD"

export WATCOM="$WATCOM_ROOT"
export PATH="$WATCOM_ROOT/binl:$PATH"
export EDPATH="$WATCOM_ROOT/eddat"
export INCLUDE="$WATCOM_ROOT/h"
export LIB="$WATCOM_ROOT/lib286"

"$WATCOM_ROOT/binl/wcl" -bt=dos -ml \
  "$ROOT/src/zine/main.c" \
  "$ROOT/src/zine/content.c" \
  "$ROOT/src/zine/preview.c" \
  "$ROOT/src/zine/scaffold.c" \
  "$ROOT/src/zine/site.c" \
  "$ROOT/src/zine/theme.c" \
  "$ROOT/src/zine/util.c" \
  -fe="$ROOT/mnt/zine.exe"

for asset in VGA.SRC WIN98.SRC CRT.SRC BANNER.TXT THEME.JS font.ttf; do
  if [ -f "$ROOT/src/zine/templates/$asset" ]; then
    cp "$ROOT/src/zine/templates/$asset" "$ROOT/mnt/$asset"
  fi
done
