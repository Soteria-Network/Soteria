// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#include "consensus.h"
#include <validation.h>

unsigned int GetMaxBlockWeight()
{
    // RIP-25: Phase 1 PQ block weight increase
    if (fPQHybridIsActive)
        return MAX_BLOCK_WEIGHT_RIP25_PHASE1;

    // Block weight for when assets weren't activated
    return MAX_BLOCK_WEIGHT;
}

unsigned int GetMaxBlockSerializedSize()
{
    // RIP-25: Phase 1 PQ block serialized size increase
    if (fPQHybridIsActive)
        return MAX_BLOCK_SERIALIZED_SIZE_RIP25_PHASE1;

    // Block serialized size for when assets weren't activated
    return MAX_BLOCK_SERIALIZED_SIZE;
}
