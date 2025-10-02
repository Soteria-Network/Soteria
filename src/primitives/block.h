// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_PRIMITIVES_BLOCK_H
#define SOTERIA_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <iostream>
#include <array>
#include <string>
#include <unordered_lru_cache.h>
#include <util/system.h>
#include <vector>

// Dual algo: An impossible pow hash (can't meet any target)
const uint256 HIGH_HASH = uint256S("0x0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
// Dual algo: Default value for -powalgo argument
const std::string DEFAULT_POW_TYPE = "soterg";
// Dual algo: Pow type names
const std::string POW_TYPE_NAMES[] = {
    "soterg",
    "soterc"
};
// Dual algo: Pow type IDs
enum POW_TYPE {
    POW_TYPE_SOTERG,
    POW_TYPE_SOTERC,
    //
    NUM_BLOCK_TYPES
};

class CBlockHeader
{
public:
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    /// Compute the SHA256 hash from the block
    uint256 GetSHA256Hash() const;

    /// Compute the PoW hash
    uint256 ComputePoWHash() const;

    /// Caching lookup/computation of POW hash
    uint256 GetHash(bool readCache = true) const;

    uint256 GetSOTERGHash() const;

    // SoterC
    static uint256 SoterCHashArbitrary(const char* data);

    /// Use for testing algo switch
    uint256 TestTiger() const;
    uint256 TestSha512() const;
    uint256 TestGost512() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    // Dual algo: Get pow type from version bits
    POW_TYPE GetPoWType() const {
        return (POW_TYPE)((nVersion >> 16) & 0xFF);
    }

    // Dual algo: Get pow type name
    std::string GetPoWTypeName() const {
        POW_TYPE pt = GetPoWType();
        if (pt >= NUM_BLOCK_TYPES)
            return "unrecognised";
        return POW_TYPE_NAMES[pt];
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }
    std::string ToString() const;
};
/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // SOTERIA_PRIMITIVES_BLOCK_H
