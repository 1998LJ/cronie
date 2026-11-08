#!/bin/sh
set -eu
tmp=$(mktemp -d "${TMPDIR:-/tmp}/cronie-crontab-env.XXXXXXXX")
trap 'rm -rf "$tmp"' 0
trap 'exit 1' 1 2 3 15
${CC:-cc} -Wall -Wextra -I. -o "$tmp/test-crontab-env" \
  src/tests/test-crontab-env.c cronie_common.c
"$tmp/test-crontab-env"
