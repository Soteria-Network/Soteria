// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developers

#ifndef SOTERIA_AMOUNT_H
#define SOTERIA_AMOUNT_H

#include <stdint.h>

typedef int64_t CAmount;

static constexpr CAmount COIN = 100000000; 
static constexpr CAmount CENT = 1000000;

static constexpr CAmount MAX_MONEY = 1500000 * COIN;  // Locked at 1.5M

inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

#endif 
