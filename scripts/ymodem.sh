#!/usr/bin/env bash
set -euo pipefail

TTY=${1:-}
BIN=${2:-}
BAUD=${3:-115200}
ADDR=${4:-0x80080000}

usage() {
  echo "Usage: $0 <tty> <binary> [baud] [addr]" >&2
  exit 1
}

[[ -n "$TTY" && -n "$BIN" ]] || usage

if [[ ! -c "$TTY" ]]; then
  echo "error: '$TTY' is not a character device" >&2
  exit 1
fi

if [[ ! -r "$BIN" ]]; then
  echo "error: binary '$BIN' not found" >&2
  exit 1
fi

if ! command -v sz >/dev/null 2>&1; then
  echo "error: 'sz' not found; run this command inside nix develop" >&2
  exit 1
fi

if command -v lsof >/dev/null 2>&1 && lsof "$TTY" >/dev/null 2>&1; then
  echo "error: '$TTY' is busy (close your serial console first)" >&2
  exit 1
fi

if [[ "${OSTYPE:-}" == darwin* ]]; then
  STTY="stty -f $TTY"
else
  STTY="stty -F $TTY"
fi

old_settings=$($STTY -g)
trap "$STTY $old_settings 2>/dev/null || true" EXIT

$STTY "$BAUD" raw -echo
printf "\rloady %s\r" "$ADDR" > "$TTY"
sleep 0.3

echo ">>> Sending '$BIN' to $TTY at $BAUD (loadaddr ${ADDR})"
sz --ymodem -b "$BIN" > "$TTY" < "$TTY"
echo "Transfer complete. Reconnect your console to continue."
