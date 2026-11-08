#!/bin/sh
# Check that MAILFROM and MAILTO expansion uses the cron entry's environment.
set -eu

tmp=$(mktemp -d "${TMPDIR:-/tmp}/cronie-expand-env.XXXXXXXX")
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' 1 2 3 15

${CC:-cc} -Wall -Wextra -I. -o "$tmp/test-expand-envvar" \
    src/tests/test-expand-envvar.c cronie_common.c
"$tmp/test-expand-envvar"
