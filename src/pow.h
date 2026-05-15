// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Copyright (c) 2025-present The Soteria Core developers

#ifndef SOTER_POW_H
#define SOTER_POW_H
#include <chain.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;


unsigned int GetNextWorkRequiredLWMA(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequiredLWMA1(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequiredLWMA2(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequiredLWMA3(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequiredLWMA4(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequiredLWMA5(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequiredLWMA6(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);

unsigned int GetStartLWMA(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType);
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::ConsensusParams&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::ConsensusParams&);

/** Check whether a block hash satisfies the proof-of-work requirement */
bool CheckProofOfWork(const CBlockHeader& blockheader, const Consensus::ConsensusParams& params, bool cache=true);

#endif
