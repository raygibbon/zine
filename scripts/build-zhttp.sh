#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD="$ROOT/build/zhttp"
HOST_HOME=${HOME:-/home/$USER}
WATCOM_ROOT=${WATCOM:-"$HOST_HOME/opt/watcom"}
WATT_ROOT=${WATT_ROOT:-"$ROOT/3rd/watt32/src"}

mkdir -p "$BUILD"
mkdir -p "$ROOT/mnt"
cd "$BUILD"

export WATCOM="$WATCOM_ROOT"
export PATH="$WATCOM_ROOT/binl:$PATH"
export EDPATH="$WATCOM_ROOT/eddat"
export INCLUDE="$WATCOM_ROOT/h:$WATT_ROOT/inc"
export LIB="$WATCOM_ROOT/lib286"

"$WATCOM_ROOT/binl/wcl" -bt=dos -ml -zc \
  -dZHTTP_USE_WATT32 -dWATT32 -dZHTTP_USE_FASTWRITE=1 \
  "$@" \
  -i="$ROOT/src/zhttp" \
  -i="$ROOT/src/shared" \
  -i="$WATT_ROOT/inc" \
  "$ROOT/src/zhttp/main.c" \
  "$ROOT/src/zhttp/http_parse.c" \
  "$ROOT/src/zhttp/http_reply.c" \
  "$ROOT/src/zhttp/serve_file.c" \
  "$ROOT/src/zhttp/mime.c" \
  "$ROOT/src/zhttp/net_watt32.c" \
  "$ROOT/src/shared/zpath.c" \
  "$ROOT/src/shared/zfile.c" \
  "$ROOT/src/shared/zlog.c" \
  "$WATT_ROOT/lib/wattcpwl.lib" \
  -fe="$ROOT/mnt/zhttp.exe"
