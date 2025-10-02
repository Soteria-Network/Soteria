// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_AMOUNT_H
#define SOTERIA_AMOUNT_H

#include <stdint.h>

/** Amount in corbies (Can be negative) */
typedef int64_t CAmount;

static constexpr CAmount COIN = 100000000; 
static constexpr CAmount CENT = 1000000;

/** No amount larger than this (in soterio) is valid.
 * 1 coin - 1000/200=5Xcoins 3s, block reward 200coins, 100coins, 5760 blocks, 86400
 * Note that this constant is *not* the total money supply, which in Soteria
 * currently happens to be less than 92,000,000,000 SOTER for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
 

/** 1. **Total Emission Until Final Phase 341,521,920 COIN.
    2. **Time Until Final Phase:** In theory ~12 years.
    3. **Final Subsidy:** 0.25 COIN/block.
   - Annual emission at final phase: 0.25 × 5,760 × 365 ≈ 525,600 COIN/year forever, ≈ 0.15% annual inflation (economically negligible) .. */
static constexpr CAmount MAX_MONEY = 92000000000 * COIN; // Summing across all intervals yields **341,521,920** coins

inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

/* Activate after Hard Fork
// 1 Billion is the max toll amount allowed when calculating the toll
static const CAmount MAX_TOLL_AMOUNT = 1000000000 * COIN;

// 0.00005000 is the minimum toll amount allowed when calculating the toll
static const CAmount MIN_TOLL_AMOUNT = 5000;
*/

#endif //  SOTERIA_AMOUNT_H
