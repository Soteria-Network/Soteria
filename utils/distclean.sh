#!/bin/sh
# Copyright (c) 2020 The Zcash core developers
# Copyright (c) 2025 The Soteria core developers

export LC_ALL=C

util/clean.sh

rm -rf depends/*-*-*
rm -rf depends/built

# These are not in clean.sh because they are only generated when building dependencies.
rm -f util/bin/db_*
rmdir util/bin 2>/dev/null || true
