// Copyright (c) 2011-2017 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-present The Soteria Core developers

#ifndef SOTERIA_RPC_BLOCKCHAIN_H
#define SOTERIA_RPC_BLOCKCHAIN_H

#include <primitives/block.h>
#include <validation.h> // for cs_main

class CBlock;
class CBlockIndex;
class UniValue;

/**
 * Get the difficulty of the net wrt to the given block index, or the chain tip if
 * not provided.
 *
 * @return A floating point number that is a multiple of the main net minimum
 * difficulty (4295032833 hashes).
 */

double GetDifficulty(const CBlockIndex* blockindex);
double GetDifficulty(POW_TYPE powType);

/** Callback for when block tip changed. */
void RPCNotifyBlockChange(bool ibd, const CBlockIndex*);

/** Block description to JSON */
UniValue blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool txDetails = false) LOCKS_EXCLUDED(cs_main);

/** Mempool information to JSON */
UniValue mempoolInfoToJSON();

/** Mempool to JSON */
UniValue mempoolToJSON(bool fVerbose = false);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex* blockindex) LOCKS_EXCLUDED(cs_main);

#endif
