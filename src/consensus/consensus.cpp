// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#include "consensus.h"

unsigned int GetMaxBlockWeight()
{
    return MAX_BLOCK_WEIGHT_RIP25_PHASE2;
}

unsigned int GetMaxBlockSerializedSize()
{
    return MAX_BLOCK_SERIALIZED_SIZE_RIP25_PHASE2;
}

bool IsPQWitnessDiscountActive(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& consensusParams)
{
    // If we later use VersionBitsState, we will replace this with that call.
    // For now, we will trust the nPQHybridEnabled flag from chainparams.
    return consensusParams.nPQHybridEnabled;
}

unsigned int GetMaxBlockWeightForPrev(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& consensusParams)
{
    // Before PQ activation, we will keep the legacy 3M limit.
    if (!IsPQWitnessDiscountActive(pindexPrev, consensusParams))
        return MAX_BLOCK_WEIGHT;

    // Once active, use the phase2 structural ceiling.
    // TODO: Add phase1 logic if we have an intermediate activation phase.
    return MAX_BLOCK_WEIGHT_RIP25_PHASE2;
}
