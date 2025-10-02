// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <string>
#include <cstdint>
#include <algo/soterc/soterc.h>
#include <algo/soterg/soterg.h>
#include <primitives/block.h>
#include <primitives/powcache.h>
#include <versionbits.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <crypto/common.h>
#include <util/system.h>
#include <sync.h>
#include <consensus/consensus.h>

// 0xFFFFFFE0/32 , 0xFFFFFFC0/64
/** GPUs need 30-45s to stabilize voltage.
CPUs require 15-25s to load algorithms.
96s provides 3 full switching cycles. PSUs: Optimal efficiency at 90-100s loads
masks = [32, 64, 96, 128], stability = [78%, 92%, 91%, 89%].
# 64s mask provides most stable hashrate, 96s Mask 2% rejected shares vs 128s Mask 5% rejected shares. */
#define TIME_MASK 0xFFFFFFA0 // 96s bitmask

uint256 CBlockHeader::GetSHA256Hash() const
{
    return SerializeHash(*this);
}

uint256 CBlockHeader::ComputePoWHash() const
{
    uint256 thash;
    unsigned int profile = 0x0;

    uint32_t nSoterGTimestamp = Params().GetConsensus().vUpgrades[Consensus::SOTERG_SWITCH].nTimestamp;
    uint32_t nSoterCTimestamp = Params().GetConsensus().vUpgrades[Consensus::SOTERC_SWITCH].nTimestamp;

    if (nTime > nSoterGTimestamp) {
        if(nTime > nSoterCTimestamp) {
            // Dual algo
            switch (GetPoWType()) {
            case POW_TYPE_SOTERG: {
                int32_t nTimeSoterG = nTime & TIME_MASK;
                uint256 hashTime = Hash(BEGIN(nTimeSoterG), END(nTimeSoterG));
                thash = HashX12R(BEGIN(nVersion), END(nNonce), hashTime);
                break;
            }
            case POW_TYPE_SOTERC: {
                return Soterc(BEGIN(nVersion), END(nNonce), true);
                break;
            }
            default: // Don't crash the client on invalid blockType, just return a bad hash
                return HIGH_HASH;
            }
        } else {
            // soterg before dual-algo
            int32_t nTimeSoterG = nTime & TIME_MASK;
            uint256 hashTime = Hash(BEGIN(nTimeSoterG), END(nTimeSoterG));
            thash = HashX12R(BEGIN(nVersion), END(nNonce), hashTime);
        }
    }
    else {
        // we keep it for testing only.
        thash = HashX12R(BEGIN(nVersion), END(nNonce), hashPrevBlock);
    }

    return thash;
}

uint256 CBlockHeader::GetHash(bool readCache) const
{
    LOCK(cs_pow);
    CPowCache& cache(CPowCache::Instance());

    uint256 headerHash = GetSHA256Hash();
    uint256 powHash;
    bool found = false;

    if (readCache) {
        found = cache.get(headerHash, powHash);
    }

    if (!found || cache.IsValidate()) {
        uint256 powHash2 = ComputePoWHash();
        if (found && powHash2 != powHash) {
           LogPrintf("PowCache failure: headerHash: %s, from cache: %s, computed: %s, correcting\n", headerHash.ToString(), powHash.ToString(), powHash2.ToString());
        }
        powHash = powHash2;
        cache.erase(headerHash); // If it exists, replace it.
        cache.insert(headerHash, powHash2);
    }
    return powHash;
}

// Soterc algo
uint256 CBlockHeader::SoterCHashArbitrary(const char* data) {
    return Soterc(data, data + strlen(data), true);
}

// soterg algo
uint256 CBlockHeader::GetSOTERGHash() const
{
    return HashX12R(BEGIN(nVersion), END(nNonce), hashPrevBlock);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}

/// Used to test algo switching between X16R and X16RV2

//uint256 CBlockHeader::TestTiger() const
//{
//    return HashTestTiger(BEGIN(nVersion), END(nNonce), hashPrevBlock);
//}
//
//uint256 CBlockHeader::TestSha512() const
//{
//    return HashTestSha512(BEGIN(nVersion), END(nNonce), hashPrevBlock);
//}
//
//uint256 CBlockHeader::TestGost512() const
//{
//    return HashTestGost512(BEGIN(nVersion), END(nNonce), hashPrevBlock);
//}

//CBlock block = GetParams().GenesisBlock();
//int64_t nStart = GetTimeMillis();
//LogPrintf("Starting Tiger %dms\n", nStart);
//block.TestTiger();
//LogPrintf("Tiger Finished %dms\n", GetTimeMillis() - nStart);
//
//nStart = GetTimeMillis();
//LogPrintf("Starting Sha512 %dms\n", nStart);
//block.TestSha512();
//LogPrintf("Sha512 Finished %dms\n", GetTimeMillis() - nStart);
//
//nStart = GetTimeMillis();
//LogPrintf("Starting Gost512 %dms\n", nStart);
//block.TestGost512();
//LogPrintf("Gost512 Finished %dms\n", GetTimeMillis() - nStart);
