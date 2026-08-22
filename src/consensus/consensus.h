// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_CONSENSUS_CONSENSUS_H
#define SOTERIA_CONSENSUS_CONSENSUS_H

#include <stdlib.h>
#include <stdint.h>
#include "chainparams.h"

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static constexpr unsigned int MAX_BLOCK_SERIALIZED_SIZE = 3000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static constexpr unsigned int MAX_BLOCK_WEIGHT = 3000000;

/** RIP-25: Phase 1 PQ block weight limit (8 MWU) */
static constexpr unsigned int MAX_BLOCK_WEIGHT_RIP25_PHASE1 = 8000000;
/** RIP-25: Phase 1 PQ block serialized size limit */
static constexpr unsigned int MAX_BLOCK_SERIALIZED_SIZE_RIP25_PHASE1 = 8000000;
/** RIP-25: Phase 2 PQ block weight limit (12 MWU) */
static constexpr unsigned int MAX_BLOCK_WEIGHT_RIP25_PHASE2 = 12000000;
/** RIP-25: Phase 2 PQ block serialized size limit */
static constexpr unsigned int MAX_BLOCK_SERIALIZED_SIZE_RIP25_PHASE2 = 12000000;

/** The maximum allowed number of signature check operations in a block (network rule) */
static constexpr int64_t MAX_BLOCK_SIGOPS_COST = 240000; // SC support // MAX_BLOCK_WEIGHT/50
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static constexpr int COINBASE_MATURITY = 4200;

/** Timestamp at which the UAHF starts. */
static constexpr uint32_t DEFAULT_UAHF_START_TIME = 2147483647; // 1760052990;Date: Tuesday, January 19, 2038, at 03:14:07 UTC

static constexpr int WITNESS_SCALE_FACTOR = 4;

/** RIP-25: PQ witness discount scale factor and per-element safety bound (8x base, PQ witness at 1/8 weight) */
static constexpr int PQ_WITNESS_SCALE_FACTOR = 8;
/** RIP-25: Maximum witness stack element size for PQ (witness v2) programs */
static constexpr unsigned int MAX_PQ_WITNESS_ELEMENT_SIZE = 4096;

static constexpr size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid CTransaction
static constexpr size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for a serialized CTransaction

#define UNUSED_VAR     __attribute__ ((unused))
//! These variables need to be in this class because undo.h use them. However because they are in this class
//! they cause unused variable warnings when compiling. This UNUSED_VAR removes the unused warnings. We can use true value to
//! correct activation flags.
UNUSED_VAR static bool fAssetsIsActive = false;
UNUSED_VAR static bool fSmartContractsIsActive = false;
UNUSED_VAR static bool fRip5IsActive = false;
UNUSED_VAR static bool fSoteriaNameSystemIsActive = false;
UNUSED_VAR static bool fTransferScriptIsActive = false;
/** Enable missing activation flags. */
UNUSED_VAR static bool fEnforcedValuesIsActive = true; // Always return true for enforced values.
UNUSED_VAR static bool fCheckCoinbaseAssetsIsActive = true; // Always return true for coinbase asset checks.

/** Structural upper bounds supported by this binary. Exact active limits are contextual. */
unsigned int GetMaxBlockWeight();
unsigned int GetMaxBlockSerializedSize();

/** RIP-25 activation/resource state for the block after pindexPrev. */
bool IsPQWitnessDiscountActive(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& consensusParams);
unsigned int GetMaxBlockWeightForPrev(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& consensusParams);

/** Flags for nSequence and nLockTime locks */
/** Interpret sequence numbers as relative lock-time constraints. */
static constexpr unsigned int LOCKTIME_VERIFY_SEQUENCE = (1 << 0);
/** Use GetMedianTimePast() instead of nTime for end point timestamp. */
static constexpr unsigned int LOCKTIME_MEDIAN_TIME_PAST = (1 << 1);

#endif // SOTERIA_CONSENSUS_CONSENSUS_H
