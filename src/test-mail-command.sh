#!/bin/sh
# Regression test for busybox-compatible -m MAILFROM substitution (issue #40).
set -eu

tmp=$(mktemp -d "${TMPDIR:-/tmp}/cronie-mailcmd.XXXXXXXX")
trap 'rm -rf "$tmp"' 0
trap 'exit 1' 1 2 3 15

${CC:-cc} -Wall -Wextra -I. -o "$tmp/test-mail-command" \
    src/tests/test-mail-command.c cronie_common.c
"$tmp/test-mail-command"
