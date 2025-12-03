// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_CONSENSUS_CONSENSUS_H
#define SOTERIA_CONSENSUS_CONSENSUS_H

#include <stdlib.h>
#include <stdint.h>
#include "chainparams.h"

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static constexpr unsigned int MAX_BLOCK_SERIALIZED_SIZE = 3000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static constexpr unsigned int MAX_BLOCK_WEIGHT = 3000000;

/** The maximum allowed weight for a block, after RIP 2 (network rule) */
//static constexpr unsigned int MAX_BLOCK_WEIGHT_RIP2 = 4000000;
// Tighter caps can push fees up under load; looser caps dampen spikes but risk “wasted” empty slots.
/** The maximum allowed size for a serialized block, in bytes after RIP 2(only for buffer size limits) */
//static constexpr unsigned int MAX_BLOCK_SERIALIZED_SIZE_RIP2 = 4000000;

/** The maximum allowed number of signature check operations in a block (network rule) */
static constexpr int64_t MAX_BLOCK_SIGOPS_COST = 240000; // SC support // MAX_BLOCK_WEIGHT/50
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static constexpr int COINBASE_MATURITY = 4200; // 60Kblocks/Epoch =8mb ~16h

/** Timestamp at which the UAHF starts. */
static constexpr uint32_t DEFAULT_UAHF_START_TIME = 2147483647; // 1760052990; until the release of Electron wallet

static constexpr int WITNESS_SCALE_FACTOR = 4;

static constexpr size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid serialized CTransaction
static constexpr size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

#define UNUSED_VAR     __attribute__ ((unused))
//! These variables need to be in this class because undo.h use them. However because they are in this class
//! they cause unused variable warnings when compiling. This UNUSED_VAR removes the unused warnings. We can use true value to
//! correct activation flags.
UNUSED_VAR static bool fAssetsIsActive = false;
UNUSED_VAR static bool fSmartContractsIsActive = false;
UNUSED_VAR static bool fSoteriaNameSystemIsActive = false;
UNUSED_VAR static bool fTransferScriptIsActive = false;
/** Enable missing activation flags. */
UNUSED_VAR static bool fEnforcedValuesIsActive = true; // Always return true for enforced values.
UNUSED_VAR static bool fCheckCoinbaseAssetsIsActive = true; // Always return true for coinbase asset checks.

unsigned int GetMaxBlockWeight();
unsigned int GetMaxBlockSerializedSize();

/** Flags for nSequence and nLockTime locks */
/** Interpret sequence numbers as relative lock-time constraints. */
static constexpr unsigned int LOCKTIME_VERIFY_SEQUENCE = (1 << 0);
/** Use GetMedianTimePast() instead of nTime for end point timestamp. */
static constexpr unsigned int LOCKTIME_MEDIAN_TIME_PAST = (1 << 1);

#endif // SOTERIA_CONSENSUS_CONSENSUS_H
