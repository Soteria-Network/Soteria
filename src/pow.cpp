// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <vector>
#include <pow.h>
#include <limits>
#include <iostream>
#include <arith_uint256.h>
#include <chain.h>
#include <array>
#include <primitives/block.h>
#include <uint256.h>
#include <string>
#include <util/system.h>
#include <validation.h>
#include <chainparams.h>
#include <tinyformat.h>

namespace {

// 1. SafeMultiply using __int128 for overflow-safety
bool SafeMultiply(int64_t a, int64_t b, int64_t& result) {
    __int128 prod = static_cast<__int128>(a) * b;
    if (prod < std::numeric_limits<int64_t>::min() ||
        prod > std::numeric_limits<int64_t>::max()) {
        return false;
    }
    result = static_cast<int64_t>(prod);
    return true;
}

} // anonymous namespace

unsigned int static DarkGravityWave(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params) {
    /* current difficulty formula, dash - DarkGravity v3, written by Evan Duffield - evan@dash.org */
    assert(pindexLast != nullptr);

    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    int64_t nPastBlocks = 180;

    // make sure we have at least (nPastBlocks + 1) blocks, otherwise just return powLimit
    if (!pindexLast || pindexLast->nHeight < nPastBlocks) {
        return bnPowLimit.GetCompact();
    }

    if (params.fPowAllowMinDifficultyBlocks && params.fPowNoRetargeting) {
        // Special difficulty rule:
        // If the new block's timestamp is more than 2 * 1 minutes
        // then allow mining of a min-difficulty block.
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2)
            return nProofOfWorkLimit;
        else {
            // Return the last non-special-min-difficulty-rules-block
            const CBlockIndex *pindex = pindexLast;
            while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 &&
                   pindex->nBits == nProofOfWorkLimit)
                pindex = pindex->pprev;
            return pindex->nBits;
        }
    }

    const CBlockIndex *pindex = pindexLast;
    arith_uint256 bnPastTargetAvg;

    for (unsigned int nCountBlocks = 1; nCountBlocks <= nPastBlocks; nCountBlocks++) {
        arith_uint256 bnTarget = arith_uint256().SetCompact(pindex->nBits);
        if (nCountBlocks == 1) {
            bnPastTargetAvg = bnTarget;
        } else {
            // NOTE: that's not an average really...
            bnPastTargetAvg = (bnPastTargetAvg * nCountBlocks + bnTarget) / (nCountBlocks + 1);
        }

        if(nCountBlocks != nPastBlocks) {
            assert(pindex->pprev); // should never fail
            pindex = pindex->pprev;
        }
    }

    arith_uint256 bnNew(bnPastTargetAvg);

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindex->GetBlockTime();
    // NOTE: is this accurate? nActualTimespan counts it for (nPastBlocks - 1) blocks only...
    int64_t nTargetTimespan = nPastBlocks * params.nPowTargetSpacing;

    if (nActualTimespan < nTargetTimespan/3)
        nActualTimespan = nTargetTimespan/3;
    if (nActualTimespan > nTargetTimespan*3)
        nActualTimespan = nTargetTimespan*3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }

    //LogPrintf("--- diff --- %d: %d\n", pindexLast->nHeight, bnNew.GetCompact());

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredBTC(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

bool IsTransitioningToSoterG(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params)
{
    if (pblock->nTime <= params.nSoterGTimestamp)
        return false;
        
    int64_t dgwWindow = 0;

    const CBlockIndex* pindex = pindexLast;
    
    while (pindex->pprev && dgwWindow > 0) {
        pindex = pindex->pprev;
        dgwWindow--;
    }
    
    return pindex->nTime <= params.nSoterGTimestamp;
}

unsigned int GetNextWorkRequiredLWMA(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
         
    // Stage-based activation by height
    if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight && pindexLast->nHeight < params.diffRetargetEndHeight)
        return GetStartLWMA(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight1 && pindexLast->nHeight < params.diffRetargetEndHeight1)
        return GetNextWorkRequiredLWMA2(pindexLast, pblock, params, powType);          
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight2 && pindexLast->nHeight < params.diffRetargetEndHeight2)
        return GetNextWorkRequiredLWMA3(pindexLast, pblock, params, powType);   
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight3 && pindexLast->nHeight < params.diffRetargetEndHeight3)
        return GetNextWorkRequiredLWMA4(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight4 && pindexLast->nHeight < params.diffRetargetEndHeight4)
        return GetNextWorkRequiredLWMA5(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight5 && pindexLast->nHeight < params.diffRetargetEndHeight5)
        return GetNextWorkRequiredLWMA6(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight6 && pindexLast->nHeight < params.diffRetargetEndHeight6)
        return GetNextWorkRequiredLWMA7(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight7 && pindexLast->nHeight < params.diffRetargetEndHeight7)
        return GetNextWorkRequiredLWMA8(pindexLast, pblock, params, powType);                  
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight8 && pindexLast->nHeight < params.diffRetargetEndHeight8)
        return GetNextWorkRequiredLWMA9(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight9 && pindexLast->nHeight < params.diffRetargetEndHeight9)
        return GetNextWorkRequiredLWMA10(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight10 && pindexLast->nHeight < params.diffRetargetEndHeight10)
        return GetNextWorkRequiredLWMA11(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight11 && pindexLast->nHeight < params.diffRetargetEndHeight11)
        return GetNextWorkRequiredLWMA12(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight12 && pindexLast->nHeight < params.diffRetargetEndHeight12)
        return GetNextWorkRequiredLWMA13(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight13 && pindexLast->nHeight < params.diffRetargetEndHeight13)
        return GetNextWorkRequiredLWMA14(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight14 && pindexLast->nHeight < params.diffRetargetEndHeight14)
        return GetNextWorkRequiredLWMA15(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight15 && pindexLast->nHeight < params.diffRetargetEndHeight15)
        return GetNextWorkRequiredLWMA16(pindexLast, pblock, params, powType);                     
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight16 && pindexLast->nHeight < params.diffRetargetEndHeight16)
        return GetNextWorkRequiredLWMA17(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight17 && pindexLast->nHeight < params.diffRetargetEndHeight17)
        return GetNextWorkRequiredLWMA18(pindexLast, pblock, params, powType); 
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight18 && pindexLast->nHeight < params.diffRetargetEndHeight18)
        return GetNextWorkRequiredLWMA19(pindexLast, pblock, params, powType);                  
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight19 && pindexLast->nHeight < params.diffRetargetEndHeight19)
        return GetNextWorkRequiredLWMA20(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight20 && pindexLast->nHeight < params.diffRetargetEndHeight20)
        return GetNextWorkRequiredLWMA21(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight21 && pindexLast->nHeight < params.diffRetargetEndHeight21)
        return GetNextWorkRequiredLWMA22(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight22 && pindexLast->nHeight < params.diffRetargetEndHeight22)
        return GetNextWorkRequiredLWMA23(pindexLast, pblock, params, powType);
    else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight23 && pindexLast->nHeight < params.diffRetargetEndHeight23)
        return GetNextWorkRequiredLWMA24(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight24 && pindexLast->nHeight < params.diffRetargetEndHeight24)
        return GetNextWorkRequiredLWMA25(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight25 && pindexLast->nHeight < params.diffRetargetEndHeight25)
        return GetNextWorkRequiredLWMA26(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight26 && pindexLast->nHeight < params.diffRetargetEndHeight26)
        return GetNextWorkRequiredLWMA27(pindexLast, pblock, params, powType);                     
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight27 && pindexLast->nHeight < params.diffRetargetEndHeight27)
        return GetNextWorkRequiredLWMA28(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight28 && pindexLast->nHeight < params.diffRetargetEndHeight28)
        return GetNextWorkRequiredLWMA29(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight29 && pindexLast->nHeight < params.diffRetargetEndHeight29)
        return GetNextWorkRequiredLWMA30(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight30 && pindexLast->nHeight < params.diffRetargetEndHeight30)
        return GetNextWorkRequiredLWMA31(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight31 && pindexLast->nHeight < params.diffRetargetEndHeight31)
        return GetNextWorkRequiredLWMA32(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight32 && pindexLast->nHeight < params.diffRetargetEndHeight32)
        return GetNextWorkRequiredLWMA33(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight33 && pindexLast->nHeight < params.diffRetargetEndHeight33)
        return GetNextWorkRequiredLWMA34(pindexLast, pblock, params, powType);   
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight34 && pindexLast->nHeight < params.diffRetargetEndHeight34)
        return GetNextWorkRequiredLWMA35(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight35 && pindexLast->nHeight < params.diffRetargetEndHeight35)
        return GetNextWorkRequiredLWMA36(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight36 && pindexLast->nHeight < params.diffRetargetEndHeight36)
        return GetNextWorkRequiredLWMA37(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight37 && pindexLast->nHeight < params.diffRetargetEndHeight37)
        return GetNextWorkRequiredLWMA38(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight38 && pindexLast->nHeight < params.diffRetargetEndHeight38)
        return GetNextWorkRequiredLWMA39(pindexLast, pblock, params, powType);
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight39 && pindexLast->nHeight < params.diffRetargetEndHeight39)
        return GetNextWorkRequiredLWMA40(pindexLast, pblock, params, powType); 
   else if (!pindexLast || pindexLast->nHeight >= params.diffRetargetStartHeight40 && pindexLast->nHeight < params.diffRetargetEndHeight40)
        return GetNextWorkRequiredLWMA41(pindexLast, pblock, params, powType);
                       
    // Fallback to default LWMA1 to keep blocks valid
    return GetNextWorkRequiredLWMA1(pindexLast, pblock, params, powType); 
}

// LWMA 90, dt 2.5, pt 1, ewma 1/4
unsigned int GetStartLWMA(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    //const arith_uint256 powTypeLimits[powType] = UintToArith256(params.powTypeLimits[POW_TYPE_SOTERG]);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 2.5 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 1; // for first blocks
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

unsigned int GetNextWorkRequiredLWMA1(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
 // const arith_uint256 powTypeLimits[powType] = UintToArith256(params.powTypeLimits[POW_TYPE_SOTERG]);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA1\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();

    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();

    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const int64_t lwmaTime = params.lwmaTimestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 40;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().nTime >= lwmaTime &&
        walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced, min=2.5
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// LWMA2 180, dt 4, pt 2, ewma 2/4
unsigned int GetNextWorkRequiredLWMA2(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4;
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

unsigned int GetNextWorkRequiredLWMA3(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
    // Basic guards
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    if (!pindexLast || !pblock) {
        return powTypeLimit.GetCompact();
    }
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        return powTypeLimit.GetCompact();
    }

    // Target spacing and window (agreed parameters)
    const int64_t T = 12;   // expected: 12 seconds
    const int64_t N = 60;                         // responsive window
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;
    const int64_t height = pindexLast->nHeight + 1;

    // Bootstrap
    if (height < N + 1) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 blocks of the same powType
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 5 * (int)N; // deeper scan to ensure enough same-type blocks

    while (walker && (int)blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        walker = walker->pprev;
        ++scanned;
    }

    if ((int)blocks.size() < N + 1) {
        // Conservative fallback: blend last target with limit so we don't snap to easiest
        arith_uint256 lastTarget; lastTarget.SetCompact(pindexLast->nBits);
        arith_uint256 fallback = (lastTarget * 3 + powTypeLimit) / 4; // 75% last, 25% limit
        return fallback.GetCompact();
    }

    // LWMA core
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound dt to [1, 4*T] (balanced cap per recommendation)
        if (dt < 1) dt = 1;
        const int64_t dtCap = 4 * T;
        if (dt > dtCap) dt = dtCap;

        const int weight = i + 1;              // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            return powTypeLimit.GetCompact();
        }

        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

    arith_uint256 avgTarget = totalTarget / N;
    arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
    arith_uint256 K = arith_uint256(k);

    // Round-to-nearest
    arith_uint256 numerator = avgTarget * weightedTime;
    arith_uint256 halfK = K >> 1;
    arith_uint256 nextTarget = (numerator + halfK) / K;

    // Clamp: allow difficulty to get up to 8× harder than limit (>>3)
    // Smaller target => harder difficulty; powTypeLimit is easiest (max target)
    arith_uint256 minTarget = powTypeLimit >> 3;
    if (nextTarget < minTarget) nextTarget = minTarget;
    if (nextTarget > powTypeLimit) nextTarget = powTypeLimit;

    // EWMA smoothing: 75% new, 25% old
    arith_uint256 lastTarget; lastTarget.SetCompact(pindexLast->nBits);
    constexpr uint64_t EWMA_NUM = 3, EWMA_DEN = 4;
    arith_uint256 smoothed = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
    if (smoothed > powTypeLimit) smoothed = powTypeLimit;

    return smoothed.GetCompact();
}

// 180, dt 4, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA4(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6;
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 180, dt 3, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA5(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 180, dt 3, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA6(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 180, dt 2.5, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA7(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 2.5 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 180, dt 2.5, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA8(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 2.5 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 120, dt 3, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA9(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 120, dt 3, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA10(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 120, dt 2.5, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA11(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 2.5 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 120, dt 2.5, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA12(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 2.5 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 90, dt 2.5, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA13(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 2.5 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 90, dt 3, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA14(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 90, dt 4, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA15(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 90, dt 4, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA16(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 120, dt 4, pt 2, ewma 1/4
unsigned int GetNextWorkRequiredLWMA17(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 4; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// 120, dt 4, pt 2, ewma 2/6
unsigned int GetNextWorkRequiredLWMA18(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 6; 
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

unsigned int GetNextWorkRequiredLWMA19(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    //const int64_t lwmaTime = params.lwma1Timestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;
        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

unsigned int GetNextWorkRequiredLWMA20(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
    // Logging throttle per POW_TYPE
    // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180; 
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    const int64_t height = pindexLast->nHeight + 1; 

    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();

    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 360;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();

    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 6 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 2;
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4;
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// soterg 6x w/o ema

unsigned int GetNextWorkRequiredLWMA21(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType) {
    const bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;                                         // Target freq
    const int64_t N = 120;                                                              // Window size
    const int64_t k = N * (N + 1) * T / 2;                                              // Constant for proper averaging after weighting solvetimes
    const int64_t height = pindexLast->nHeight + 1;                                     // Block height

    // Not enough blocks on chain? Return limit
    if (height < N) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Allowing %s pow type limit (short chain)\n", POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0, blocksFound = 0;

    // Find previousTimestamp (N blocks of this blocktype back) 
    const CBlockIndex* blockPreviousTimestamp = pindexLast;
    while (blocksFound < N) {
     
        // Wrong block type? Skip
        if (blockPreviousTimestamp->GetBlockHeader().GetPoWType() != powType) {
            assert (blockPreviousTimestamp->pprev);
            blockPreviousTimestamp = blockPreviousTimestamp->pprev;
            continue;
        }

        blocksFound++;
        if (blocksFound == N)   // Don't step to next one if we're at the one we want
            break;

        assert (blockPreviousTimestamp->pprev);
        blockPreviousTimestamp = blockPreviousTimestamp->pprev;
    }
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();
    if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: previousTime: First in period is %s at height %i\n", blockPreviousTimestamp->GetBlockHeader().GetHash().ToString().c_str(), blockPreviousTimestamp->nHeight);

    // Find N most recent blocks of wanted type
    blocksFound = 0;
    while (blocksFound < N) {
        // Wrong block type? Skip
        if (pindexLast->GetBlockHeader().GetPoWType() != powType) {
            assert (pindexLast->pprev);
            pindexLast = pindexLast->pprev;
            continue;
        }

        const CBlockIndex* block = pindexLast;
        blocksFound++;
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? block->GetBlockTime() : previousTimestamp + 1;

        // 6*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);

        // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;

        // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.

        // Now step!
        assert (pindexLast->pprev);
        pindexLast = pindexLast->pprev;            
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Allowing %s pow type limit (target too high)\n", POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }

    return nextTarget.GetCompact();
}

// soterg 10x w/o ema

unsigned int GetNextWorkRequiredLWMA22(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType) {

    const bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const int64_t k = (N * (N + 1)) / 2 * T;  // Sum of weights * target time (488,700)
    const int64_t height = pindexLast->nHeight + 1;                                 // Block height, last + 1, since we are finding next block diff
    
    // ===== Rate Limiting for Logs =====
    static std::map<POW_TYPE, int64_t> lastLogTime;
    const int64_t now = GetTime();
    const bool logThisCall = verbose && (now - lastLogTime[powType] > 60);
    if (logThisCall) lastLogTime[powType] = now;
    
    // ===== Input Validation =====
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact(); 
    }
    
    // ===== Chain Continuity Check =====
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain in GetNextWorkRequiredLWMA3 (height %d)\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Need N+1 blocks for N solve times
    if (height < N + 1) {
        if (logThisCall) LogPrintf("* Allowing %s pow type limit (short chain, height=%d < %d)\n", 
                                  POW_TYPE_NAMES[powType], height, N+1);
        return powTypeLimit.GetCompact();
    }

    // Collect N+1 consecutive blocks of this POW type
    std::vector<const CBlockIndex*> blocks;
    const CBlockIndex* blockWalker = pindexLast;
    int collected = 0;
    
    while (collected < (N + 1) && blockWalker) {
 {
            if (blockWalker->pprev) blockWalker = blockWalker->pprev;
            continue;
        }
        
        // Only collect matching POW types
        if (blockWalker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(blockWalker);
            collected++;
        }
        blockWalker = blockWalker->pprev;
    }

    // Check if we have enough blocks
    if (blocks.size() < N + 1) {
        if (logThisCall) LogPrintf("* Only found %d/%d blocks for %s\n", blocks.size(), N+1, POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }

    // ===== Safe Arithmetic =====
    arith_uint256 totalTarget = 0;
    int64_t totalWeightedSolveTime = 0;
    int64_t prevTime = blocks[N]->GetBlockTime();  // Oldest block timestamp
    
    for (int i = N - 1; i >= 0; i--) {
        const CBlockIndex* block = blocks[i];
        
        // Calculate solve time with bounds
        int64_t solveTime = block->GetBlockTime() - prevTime;
        if (solveTime <= 0) solveTime = 1;
        if (solveTime > 10 * T) solveTime = 10 * T;
        prevTime = block->GetBlockTime();

        // Assign weights (newest block = highest weight)
        int weight = N - i;  // i=0 (oldest): weight=1, i=N-1 (newest): weight=N
        
        // Safe multiplication
        int64_t weightedTime;
        if (!SafeMultiply(solveTime, weight, weightedTime)) {
            if (logThisCall) LogPrintf("ERROR: Overflow in weight calculation at block %d\n", block->nHeight);
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += weightedTime;

        // Accumulate target (using nBits)
        arith_uint256 target;
        target.SetCompact(block->nBits);
        totalTarget += target;
    }

    // ===== Difficulty Calculation =====
    arith_uint256 avgTarget = totalTarget / N;
    arith_uint256 nextTarget = (avgTarget * totalWeightedSolveTime) / k;

    // ===== Final Clamping =====
    if (nextTarget > powTypeLimit) {
        if (logThisCall) LogPrintf("* Clamping %s target to limit\n", POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }
    return nextTarget.GetCompact();
}

// soterg 4x w/o ema
unsigned int GetNextWorkRequiredLWMA23(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType) {
    const bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;                                 // Target freq
    const int64_t N = 120;                                                          // Window size
    const int64_t k = N * (N + 1) * T / 2;                                          // Constant for proper averaging after weighting solvetimes
    const int64_t height = pindexLast->nHeight + 1;                                 // Block height, last + 1, since we are finding next block diff

    // Not enough blocks on chain? Return limit
    if (height < N) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Allowing %s pow limit (short chain)\n", POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }
    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0, blocksFound = 0;

    // Find previousTimestamp (N blocks of this blocktype back) 
    const CBlockIndex* blockPreviousTimestamp = pindexLast;
    while (blocksFound < N) {
     
        // Wrong block type? Skip
        if (blockPreviousTimestamp->GetBlockHeader().GetPoWType() != powType) {
            assert (blockPreviousTimestamp->pprev);
            blockPreviousTimestamp = blockPreviousTimestamp->pprev;
            continue;
        }

        blocksFound++;
        if (blocksFound == N)   // Don't step to next one if we're at the one we want
            break;

        assert (blockPreviousTimestamp->pprev);
        blockPreviousTimestamp = blockPreviousTimestamp->pprev;
    }
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();
    if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: previousTime: First in period is %s at height %i\n", blockPreviousTimestamp->GetBlockHeader().GetHash().ToString().c_str(), blockPreviousTimestamp->nHeight);

    // Find N most recent blocks of wanted type
    blocksFound = 0;
    while (blocksFound < N) {
        // Wrong block type? Skip
        if (pindexLast->GetBlockHeader().GetPoWType() != powType) {
            assert (pindexLast->pprev);
            pindexLast = pindexLast->pprev;
            continue;
        }

        const CBlockIndex* block = pindexLast;
        blocksFound++;
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? block->GetBlockTime() : previousTimestamp + 1;

        // 4*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(4* T, thisTimestamp - previousTimestamp);

        // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;

        // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.

        // Now step!
        assert (pindexLast->pprev);
        pindexLast = pindexLast->pprev;            
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Allowing %s pow type limit (target too high)\n", POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }

    return nextTarget.GetCompact();
}

// soterg 6x 90
unsigned int GetNextWorkRequiredLWMA24(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
	const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
	const int64_t T = params.nPowTargetSpacing;                                      // Target freq 12s
	const int64_t N = 90;                                                           // Window size - 90 as per zawy12 graphs
	const int64_t k = N * (N + 1) * T / 2;                                          // Constant for proper averaging after weighting solvetimes == 109800
	const int64_t height = pindexLast->nHeight + 1;                                 // Block height, last + 1, since we are finding next block diff

	arith_uint256 sum_target;
	int64_t t = 0, j = 0;
	int64_t solvetime = 0;

	std::vector<const CBlockIndex*> SameAlgoBlocks;
	for (int c = height-1; SameAlgoBlocks.size() < (N + 1); c--){
		const CBlockIndex* block = pindexLast->GetAncestor(c); // -1 after execution
		if (block->GetBlockHeader().GetPoWType() == powType){
			SameAlgoBlocks.push_back(block);
		}
		if (c < 100){ // If there are not enough blocks with this algo
			return powTypeLimit.GetCompact();
		}
	}

	// Loop through N most recent blocks. starting with the lowest blockheight
	for (int i = N; i > 0; i--) {
		const CBlockIndex* block = SameAlgoBlocks[i-1];
		const CBlockIndex* block_Prev = SameAlgoBlocks[i];

		solvetime = block->GetBlockTime() - block_Prev->GetBlockTime();
		// solvetime is always min 1 second, max 360s to avoid huge variances and negative timestamps
		solvetime = std::min(solvetime, 6*T);
		if (solvetime < 1)
			solvetime = 1;

		j++;
		t += solvetime * j;  // Weighted solvetime sum.
		arith_uint256 target;
		target.SetCompact(block->nBits);
		sum_target += target / N / k;
	}
	arith_uint256 next_target = t * sum_target;
	if (next_target > powTypeLimit) {
		next_target = powTypeLimit;
	}
	return next_target.GetCompact();
}

// soterg 6x 90 w/o ema
unsigned int GetNextWorkRequiredLWMA25(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
    const bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t N = 90;                                                           // Window size
    const int64_t k = 1263;                                                         // Constant for proper averaging after weighting solvetimes (k=(N+1)/2*TargetSolvetime*0.998)
    const int64_t height = pindexLast->nHeight + 1;                                 // Block height, last + 1, since we are finding next block diff
    assert(height > N);
    
    if (params.fPowNoRetargeting) {
        return pindexLast->nBits;
    }
    arith_uint256 sum_target;
    int t = 0, j = 0, blocksFound = 0;
    // Loop through N most recent blocks.
    for (int i = height - N; i < height; i++) {
        if (pindexLast->GetBlockHeader().GetPoWType() != powType) {
            if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Height %i: Skipping %s (wrong blocktype)\n", pindexLast->nHeight, pindexLast->GetBlockHeader().GetHash().ToString().c_str());
            assert (pindexLast->pprev);
            pindexLast = pindexLast->pprev;
            continue;
        } else {
            blocksFound++;
        }

        const CBlockIndex* block = pindexLast->GetAncestor(i);
        const CBlockIndex* block_Prev = block->GetAncestor(i - 1);
        if(block == nullptr || block_Prev == nullptr) {
            assert (pindexLast->pprev);
            pindexLast = pindexLast->pprev;
            continue;   
        }

        int64_t solvetime = block->GetBlockTime() - block_Prev->GetBlockTime();
        j++;
        t += solvetime * j; // Weighted solvetime sum.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sum_target += target / arith_uint256(k * N * N);
    }

    // Keep t reasonable in case strange solvetimes occurred.
    if (t < N * k / 3) {
        t = N * k / 3;
    }

    arith_uint256 next_target = t * sum_target;
    if (next_target > powTypeLimit) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Allowing %s pow type limit (target too high)\n", POW_TYPE_NAMES[powType]);
        next_target = powTypeLimit;
    }

    if(blocksFound == 0){
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA3: Allowing %s pow type limit (blocksFound returned 0)\n", POW_TYPE_NAMES[powType]);
        return powTypeLimit.GetCompact();
    }
    return next_target.GetCompact();
}

// soterg 180-180 blocks weight 25%
unsigned int GetNextWorkRequiredLWMA26(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
 // const arith_uint256 powTypeLimits[powType] = UintToArith256(params.powTypeLimits[POW_TYPE_SOTERG]);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 180;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 3; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// LWMA16 soterg 180-300 blocks 25% weight
unsigned int GetNextWorkRequiredLWMA27(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
 // const arith_uint256 powTypeLimits[powType] = UintToArith256(params.powTypeLimits[POW_TYPE_SOTERG]);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();

    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();

    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();

        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    arith_uint256 minTarget = powTypeLimit >> 3; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }
    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }
    return smoothed.GetCompact();
}

// soterg 180-180 blocks weight 50%
unsigned int GetNextWorkRequiredLWMA28(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = 120;
    const int64_t N = params.lwmaAveragingWindow;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 180;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 3; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// soterg 180-300 blocks 50% weight
unsigned int GetNextWorkRequiredLWMA29(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
 // const arith_uint256 powTypeLimits[powType] = UintToArith256(params.powTypeLimits[POW_TYPE_SOTERG]);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 90;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 4 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 3; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// soterg 180-180 blocks weight 50% 6x
unsigned int GetNextWorkRequiredLWMA30(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 120;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 180;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 6 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to 
    arith_uint256 minTarget = powTypeLimit >> 4; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }
    return smoothed.GetCompact();
}

// soterg 180-300 blocks 50% weight
unsigned int GetNextWorkRequiredLWMA31(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
 // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60); // def=30
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

 // Parameters
 // const arith_uint256 powTypeLimits[powType] = UintToArith256(params.powTypeLimits[POW_TYPE_SOTERG]);
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = 180;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight + 1;
    
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }
    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        }
        return powTypeLimit.GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        // Early break if too far below fork height
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();
    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();
        // Bound solve time to [1, 4*T], old practice 6, new 4, if 2 is conservative we can increase it to 3
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 6 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

   // Compute avgTarget
   arith_uint256 avgTarget = totalTarget / N;

   // Build up numerator and denominator as 256-bit words
   arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
   arith_uint256 K            = arith_uint256(k);

   // Round-to-nearest: (numerator + K/2) / K
   // This avoids the “always-floor” bias of integer division.
   arith_uint256 numerator    = avgTarget * weightedTime;
   arith_uint256 halfK        = K >> 1;  // same as K/2 for unsigned
   arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 4; // 2
  // no harder than 2× the limit, def =2, 1 = 2× harder, 2 = 4× harder, 3 = 8× harder, 4 = 16× harder
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* Clamping target to powTypeLimit\n");
        nextTarget = powTypeLimit;
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 2, EWMA_DEN = 4; // 2 , 4
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) {
        smoothed = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// use soterg as default
unsigned int GetNextWorkRequiredLWMA32(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{  

    const int64_t T = 12;
   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = 180;
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 
    const int64_t height = pindexLast->nHeight;
    
   // New coins just "give away" first N blocks. It's better to guess
   // this value instead of using powTypeLimit, but err on high side to not get stuck.
    if (height < N) { return powTypeLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // Do not attempt anything like  if (solvetime < 1) {solvetime=1;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // 6*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(4 * T, thisTimestamp - previousTimestamp);

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) { nextTarget = powTypeLimit; }

    return nextTarget.GetCompact();
}

// use soterg as a min default with 6x loose value & 4x harder w/o smoothing
unsigned int GetNextWorkRequiredLWMA33(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{    
    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow; 
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;
    const int64_t height = pindexLast->nHeight + 1;
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60);
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    if (N <= 0 || T <= 0) {
        if (logThis) LogPrintf("ERROR: invalid N or T N=%lld T=%lld\n", (long long)N, (long long)T);
        return powTypeLimit.GetCompact();
    }
    
    if (height < N + 1) {
        if (logThis) LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks (no tiny maxDepth)
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);
    const CBlockIndex* walker = pindexLast;
    while (walker && blocks.size() < (size_t)(N + 1)) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        walker = walker->pprev;
    }

    if (blocks.size() < (size_t)(N + 1)) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d same-type blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Ensure blocks vector is ordered oldest..newest for ease of calc
    std::reverse(blocks.begin(), blocks.end()); // now blocks[0] is oldest, blocks[N] is newest

    // Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;

    // prevTime = time of block i-1, start with blocks[0]
    int64_t prevTime = blocks[0]->GetBlockTime();
    for (int i = 1; i <= N; ++i) {
        const CBlockIndex* b = blocks[i]; // i from 1..N, weight = i
        int64_t curTime = b->GetBlockTime();
        int64_t dt = curTime - prevTime;
        prevTime = curTime;

        // Bound solve time to [1, 6*T]
        int64_t upper = 6 * T;
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, upper));
        int64_t weight = i; // 1..N

        // Accumulate weight*dt using 128-bit
        __int128 wdt = (__int128)dt * (__int128)weight;
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

    if (totalWeightedSolveTime <= 0) {
        if (logThis) LogPrintf("ERROR: totalWeightedSolveTime non-positive (%lld)\n", (long long)totalWeightedSolveTime);
        return powTypeLimit.GetCompact();
    }

    arith_uint256 avgTarget = totalTarget / N;
    arith_uint256 weightedTime = arith_uint256((unsigned __int128) totalWeightedSolveTime);
    arith_uint256 K = arith_uint256((unsigned __int128) k);

    if (K == 0) {
        if (logThis) LogPrintf("ERROR: K == 0\n");
        return powTypeLimit.GetCompact();
    }

    // Round-to-nearest
    arith_uint256 numerator = avgTarget * weightedTime;
    arith_uint256 halfK = K >> 1;
    arith_uint256 nextTarget = (numerator + halfK) / K;

    // will allow reasonable hardness change and loosened while testing
    arith_uint256 minTarget = powTypeLimit >> 2; // allow up to 4x harder than powTypeLimit (tunable)
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* nextTarget below minTarget, raising\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* nextTarget above powTypeLimit, clamping\n");
        nextTarget = powTypeLimit;
    }

    // For testability disable EWMA smoothing; re-enable if desired 25% or 50% weight
    bool useEWMA = false;
    arith_uint256 finalTarget = nextTarget;
    if (useEWMA) {
        arith_uint256 lastTarget;
        lastTarget.SetCompact(pindexLast->nBits);
        constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8;
        finalTarget = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
        if (finalTarget > powTypeLimit) finalTarget = powTypeLimit;
    }
    if (logThis) {
        LogPrintf("* LWMA N=%lld T=%lld totalWeightedSolveTime=%s nextTarget=%s finalTarget=%s\n",
                  (long long)N, (long long)T,
                  arith_uint256((unsigned __int128)totalWeightedSolveTime).ToString(),
                  nextTarget.ToString(), finalTarget.ToString());
    }
    return finalTarget.GetCompact();
}

// use soterg as a min default with 4x loose value & 8x harder
unsigned int GetNextWorkRequiredLWMA34(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{    
    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;
    const int64_t height = pindexLast->nHeight + 1;

    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60);
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    if (N <= 0 || T <= 0) {
        if (logThis) LogPrintf("ERROR: invalid N or T N=%lld T=%lld\n", (long long)N, (long long)T);
        return powTypeLimit.GetCompact();
    }

    if (height < N + 1) {
        if (logThis) LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks (no tiny maxDepth)
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);
    const CBlockIndex* walker = pindexLast;
    while (walker && blocks.size() < (size_t)(N + 1)) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        walker = walker->pprev;
    }

    if (blocks.size() < (size_t)(N + 1)) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d same-type blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Ensure blocks vector is ordered oldest..newest for ease of calc
    std::reverse(blocks.begin(), blocks.end()); // now blocks[0] is oldest, blocks[N] is newest

    // Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;

    // prevTime = time of block i-1, start with blocks[0]
    int64_t prevTime = blocks[0]->GetBlockTime();
    for (int i = 1; i <= N; ++i) {
        const CBlockIndex* b = blocks[i]; // i from 1..N, weight = i
        int64_t curTime = b->GetBlockTime();
        int64_t dt = curTime - prevTime;
        prevTime = curTime;

        // Bound solve time to [1, 4*T]
        int64_t upper = 4 * T;
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, upper));
        int64_t weight = i; // 1..N

        // Accumulate weight*dt using 128-bit
        __int128 wdt = (__int128)dt * (__int128)weight;
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

    if (totalWeightedSolveTime <= 0) {
        if (logThis) LogPrintf("ERROR: totalWeightedSolveTime non-positive (%lld)\n", (long long)totalWeightedSolveTime);
        return powTypeLimit.GetCompact();
    }

    arith_uint256 avgTarget = totalTarget / N;
    arith_uint256 weightedTime = arith_uint256((unsigned __int128) totalWeightedSolveTime);
    arith_uint256 K = arith_uint256((unsigned __int128) k);

    if (K == 0) {
        if (logThis) LogPrintf("ERROR: K == 0\n");
        return powTypeLimit.GetCompact();
    }

    // Round-to-nearest
    arith_uint256 numerator = avgTarget * weightedTime;
    arith_uint256 halfK = K >> 1;
    arith_uint256 nextTarget = (numerator + halfK) / K;

    // will allow reasonable hardness change and loosened while testing
    arith_uint256 minTarget = powTypeLimit >> 3; // allow up to 8x harder than powTypeLimit (tunable)
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* nextTarget below minTarget, raising\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* nextTarget above powTypeLimit, clamping\n");
        nextTarget = powTypeLimit;
    }

    // For testability disable EWMA smoothing; re-enable if desired 25%1/8 or 50%2/4 weight
    bool useEWMA = false;
    arith_uint256 finalTarget = nextTarget;
    if (useEWMA) {
        arith_uint256 lastTarget;
        lastTarget.SetCompact(pindexLast->nBits);
        constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8;
        finalTarget = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
        if (finalTarget > powTypeLimit) finalTarget = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* LWMA N=%lld T=%lld totalWeightedSolveTime=%s nextTarget=%s finalTarget=%s\n",
                  (long long)N, (long long)T,
                  arith_uint256((unsigned __int128)totalWeightedSolveTime).ToString(),
                  nextTarget.ToString(), finalTarget.ToString());
    }

    return finalTarget.GetCompact();
}

// use soterg as default
unsigned int GetNextWorkRequiredLWMA35(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{  

    const int64_t T = 12;
   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = 180;
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 

    const int64_t height = pindexLast->nHeight;
    
   // New coins just "give away" first N blocks. It's better to guess
   // this value instead of using powTypeLimit, but err on high side to not get stuck.
    if (height < N) { return powTypeLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // Do not attempt anything like  if (solvetime < 1) {solvetime=1;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // 6*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) { nextTarget = powTypeLimit; }

    return nextTarget.GetCompact();
}

// use 0000000fff as default diff for both algos
unsigned int GetNextWorkRequiredLWMA36(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{  

    const int64_t T = 12;
   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = 180;
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 

    const int64_t height = pindexLast->nHeight + 1;
    
   // New coins just "give away" first N blocks. It's better to guess
   // this value instead of using powTypeLimit, but err on high side to not get stuck.
    if (height < N) { return powTypeLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // Do not attempt anything like  if (solvetime < 1) {solvetime=1;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // 6*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) { nextTarget = powTypeLimit; }

    return nextTarget.GetCompact();
}

// soterg 4x 120
unsigned int GetNextWorkRequiredLWMA37(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
	const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
	const int64_t T = params.nPowTargetSpacing * 2;                                 // Target freq 12s x 2 algos
	const int64_t N = 120;                                                           // Window size - 60 as per zawy12 graphs
	const int64_t k = N * (N + 1) * T / 2;                                          // Constant for proper averaging after weighting solvetimes == 109800
	const int64_t height = pindexLast->nHeight + 1;                                 // Block height, last + 1, since we are finding next block diff

	arith_uint256 sum_target;
	int64_t t = 0, j = 0;
	int64_t solvetime = 0;

	std::vector<const CBlockIndex*> SameAlgoBlocks;
	for (int c = height-1; SameAlgoBlocks.size() < (N + 1); c--){
		const CBlockIndex* block = pindexLast->GetAncestor(c); // -1 after execution
		if (block->GetBlockHeader().GetPoWType() == powType){
			SameAlgoBlocks.push_back(block);
		}

		if (c < 100){ // If there are not enough blocks with this algo, return powTypeLimit until dataset is big enough
			return powTypeLimit.GetCompact();
		}
	}

	// Loop through N most recent blocks. starting with the lowest blockheight
	for (int i = N; i > 0; i--) {
		const CBlockIndex* block = SameAlgoBlocks[i-1];
		const CBlockIndex* block_Prev = SameAlgoBlocks[i];

		solvetime = block->GetBlockTime() - block_Prev->GetBlockTime();
		// solvetime is always min 1 second, max 360s to avoid huge variances and negative timestamps
		solvetime = std::min(solvetime, 4*T);
		if (solvetime < 1)
			solvetime = 1;

		j++;
		t += solvetime * j;  // Weighted solvetime sum.
		arith_uint256 target;
		target.SetCompact(block->nBits);
		sum_target += target / N / k;
	}

	arith_uint256 next_target = t * sum_target;

	if (next_target > powTypeLimit) {
		next_target = powTypeLimit;
	}

	return next_target.GetCompact();
}

// use soterg as default 4t
unsigned int GetNextWorkRequiredLWMA38(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{  

    const int64_t T = 12;
   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = 180;
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 

    const int64_t height = pindexLast->nHeight;
    
   // New coins just "give away" first N blocks. It's better to guess
   // this value instead of using powTypeLimit, but err on high side to not get stuck.
    if (height < N) { return powTypeLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // Do not attempt anything like  if (solvetime < 1) {solvetime=1;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // 6*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(4 * T, thisTimestamp - previousTimestamp);

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) { nextTarget = powTypeLimit; }

    return nextTarget.GetCompact();
}

// use 0000000fff as default diff for both algos 4t
unsigned int GetNextWorkRequiredLWMA39(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{  

    const int64_t T = 12;
   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = 180;
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 

    const int64_t height = pindexLast->nHeight + 1;
    
   // New coins just "give away" first N blocks. It's better to guess
   // this value instead of using powTypeLimit, but err on high side to not get stuck.
    if (height < N) { return powTypeLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // Do not attempt anything like  if (solvetime < 1) {solvetime=1;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // 6*T limit prevents large drops in diff from long solvetimes which would cause oscillations.
        int64_t solvetime = std::min(4 * T, thisTimestamp - previousTimestamp);

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powTypeLimit) { nextTarget = powTypeLimit; }

    return nextTarget.GetCompact();
}

// use soterg as a min default with 6x loose value & 8x harder w/o smoothing
unsigned int GetNextWorkRequiredLWMA40(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{    
    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow; 
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;
    const int64_t height = pindexLast->nHeight + 1;
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA3\n");
        return powTypeLimit.GetCompact();
    }

    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60);
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    if (N <= 0 || T <= 0) {
        if (logThis) LogPrintf("ERROR: invalid N or T N=%lld T=%lld\n", (long long)N, (long long)T);
        return powTypeLimit.GetCompact();
    }
    
    if (height < N + 1) {
        if (logThis) LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks (no tiny maxDepth)
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);
    const CBlockIndex* walker = pindexLast;
    while (walker && blocks.size() < (size_t)(N + 1)) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        walker = walker->pprev;
    }

    if (blocks.size() < (size_t)(N + 1)) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d same-type blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Ensure blocks vector is ordered oldest..newest for ease of calc
    std::reverse(blocks.begin(), blocks.end()); // now blocks[0] is oldest, blocks[N] is newest

    // Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;

    // prevTime = time of block i-1, start with blocks[0]
    int64_t prevTime = blocks[0]->GetBlockTime();
    for (int i = 1; i <= N; ++i) {
        const CBlockIndex* b = blocks[i]; // i from 1..N, weight = i
        int64_t curTime = b->GetBlockTime();
        int64_t dt = curTime - prevTime;
        prevTime = curTime;

        // Bound solve time to [1, 6*T]
        int64_t upper = 6 * T;
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, upper));
        int64_t weight = i; // 1..N

        // Accumulate weight*dt using 128-bit
        __int128 wdt = (__int128)dt * (__int128)weight;
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

    if (totalWeightedSolveTime <= 0) {
        if (logThis) LogPrintf("ERROR: totalWeightedSolveTime non-positive (%lld)\n", (long long)totalWeightedSolveTime);
        return powTypeLimit.GetCompact();
    }

    arith_uint256 avgTarget = totalTarget / N;
    arith_uint256 weightedTime = arith_uint256((unsigned __int128) totalWeightedSolveTime);
    arith_uint256 K = arith_uint256((unsigned __int128) k);

    if (K == 0) {
        if (logThis) LogPrintf("ERROR: K == 0\n");
        return powTypeLimit.GetCompact();
    }

    // Round-to-nearest
    arith_uint256 numerator = avgTarget * weightedTime;
    arith_uint256 halfK = K >> 1;
    arith_uint256 nextTarget = (numerator + halfK) / K;

    // will allow reasonable hardness change and loosened while testing
    arith_uint256 minTarget = powTypeLimit >> 3; // allow up to 8x harder than powTypeLimit (tunable)
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* nextTarget below minTarget, raising\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* nextTarget above powTypeLimit, clamping\n");
        nextTarget = powTypeLimit;
    }

    // For testability disable EWMA smoothing; re-enable if desired 25% or 50% weight
    bool useEWMA = false;
    arith_uint256 finalTarget = nextTarget;
    if (useEWMA) {
        arith_uint256 lastTarget;
        lastTarget.SetCompact(pindexLast->nBits);
        constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8;
        finalTarget = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
        if (finalTarget > powTypeLimit) finalTarget = powTypeLimit;
    }
    if (logThis) {
        LogPrintf("* LWMA N=%lld T=%lld totalWeightedSolveTime=%s nextTarget=%s finalTarget=%s\n",
                  (long long)N, (long long)T,
                  arith_uint256((unsigned __int128)totalWeightedSolveTime).ToString(),
                  nextTarget.ToString(), finalTarget.ToString());
    }
    return finalTarget.GetCompact();
}

// use soterg as a min default with 4x loose value & 4x harder than powTypeLimit
unsigned int GetNextWorkRequiredLWMA41(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{    
    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;
    const int64_t height = pindexLast->nHeight + 1;

    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA1\n");
        return powTypeLimit.GetCompact();
    }

    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60);
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    if (N <= 0 || T <= 0) {
        if (logThis) LogPrintf("ERROR: invalid N or T N=%lld T=%lld\n", (long long)N, (long long)T);
        return powTypeLimit.GetCompact();
    }

    if (height < N + 1) {
        if (logThis) LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks (no tiny maxDepth)
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);
    const CBlockIndex* walker = pindexLast;
    while (walker && blocks.size() < (size_t)(N + 1)) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        walker = walker->pprev;
    }

    if (blocks.size() < (size_t)(N + 1)) {
        if (logThis) {
            LogPrintf("* Found only %zu/%d same-type blocks, using powTypeLimit\n",
                      blocks.size(), N+1);
        }
        return powTypeLimit.GetCompact();
    }

    // Ensure blocks vector is ordered oldest..newest for ease of calc
    std::reverse(blocks.begin(), blocks.end()); // now blocks[0] is oldest, blocks[N] is newest

    // Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;

    // prevTime = time of block i-1, start with blocks[0]
    int64_t prevTime = blocks[0]->GetBlockTime();
    for (int i = 1; i <= N; ++i) {
        const CBlockIndex* b = blocks[i]; // i from 1..N, weight = i
        int64_t curTime = b->GetBlockTime();
        int64_t dt = curTime - prevTime;
        prevTime = curTime;

        // Bound solve time to [1, 4*T]
        int64_t upper = 4 * T;
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, upper));
        int64_t weight = i; // 1..N

        // Accumulate weight*dt using 128-bit
        __int128 wdt = (__int128)dt * (__int128)weight;
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

    if (totalWeightedSolveTime <= 0) {
        if (logThis) LogPrintf("ERROR: totalWeightedSolveTime non-positive (%lld)\n", (long long)totalWeightedSolveTime);
        return powTypeLimit.GetCompact();
    }

    arith_uint256 avgTarget = totalTarget / N;
    arith_uint256 weightedTime = arith_uint256((unsigned __int128) totalWeightedSolveTime);
    arith_uint256 K = arith_uint256((unsigned __int128) k);

    if (K == 0) {
        if (logThis) LogPrintf("ERROR: K == 0\n");
        return powTypeLimit.GetCompact();
    }

    // Round-to-nearest
    arith_uint256 numerator = avgTarget * weightedTime;
    arith_uint256 halfK = K >> 1;
    arith_uint256 nextTarget = (numerator + halfK) / K;

    // will allow reasonable hardness change and loosened while testing
    arith_uint256 minTarget = powTypeLimit >> 2; // allow up to 4x harder than powTypeLimit (tunable)
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* nextTarget below minTarget, raising\n");
        nextTarget = minTarget;
    }
    if (nextTarget > powTypeLimit) {
        if (logThis) LogPrintf("* nextTarget above powTypeLimit, clamping\n");
        nextTarget = powTypeLimit;
    }

    // For testability disable EWMA smoothing; re-enable if desired 25%1/8 or 50%2/4 weight
    bool useEWMA = false;
    arith_uint256 finalTarget = nextTarget;
    if (useEWMA) {
        arith_uint256 lastTarget;
        lastTarget.SetCompact(pindexLast->nBits);
        constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8;
        finalTarget = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
        if (finalTarget > powTypeLimit) finalTarget = powTypeLimit;
    }

    if (logThis) {
        LogPrintf("* LWMA N=%lld T=%lld totalWeightedSolveTime=%s nextTarget=%s finalTarget=%s\n",
                  (long long)N, (long long)T,
                  arith_uint256((unsigned __int128)totalWeightedSolveTime).ToString(),
                  nextTarget.ToString(), finalTarget.ToString());
    }

    return finalTarget.GetCompact();
}

// Call correct diff adjust for blocks prior to SoterC Algo
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params)
{
    int dgw = DarkGravityWave(pindexLast, pblock, params);
    int btc = GetNextWorkRequiredBTC(pindexLast, pblock, params);
    int64_t nPrevBlockTime = (pindexLast->pprev ? pindexLast->pprev->GetBlockTime() : pindexLast->GetBlockTime());

    if (IsDGWActive(pindexLast->nHeight + 1)) {
        LogPrint(BCLog::NET, "Block %s - version: %s: found next work required using DGW: [%s] (BTC would have been [%s]\t(%+d)\t(%0.3f%%)\t(%s sec))\n",
                 pindexLast->nHeight + 1, pblock->nVersion, dgw, btc, btc - dgw, (float)(btc - dgw) * 100.0 / (float)dgw, pindexLast->GetBlockTime() - nPrevBlockTime);
        return dgw;
    }
    else {
        LogPrint(BCLog::NET, "Block %s - version: %s: found next work required using BTC: [%s] (DGW would have been [%s]\t(%+d)\t(%0.3f%%)\t(%s sec))\n",
                  pindexLast->nHeight + 1, pblock->nVersion, btc, dgw, dgw - btc, (float)(dgw - btc) * 100.0 / (float)btc, pindexLast->GetBlockTime() - nPrevBlockTime);
        return btc;
    }
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::ConsensusParams& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWorkSoterC(uint256 hash, unsigned int nBits, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
	// SoterC active with LWMA3 as retargetter and different powtargets/algo
	bool fNegative;
	bool fOverflow;
	arith_uint256 bnTarget;

	bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

	// Check range
	if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powTypeLimits[powType]))
		return false;

	// Check proof of work matches claimed amount
	if (UintToArith256(hash) > bnTarget)
		return false;

	return true;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::ConsensusParams& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

bool CheckProofOfWork(const CBlockHeader& blockheader, const Consensus::ConsensusParams& params, bool cache)
{
	if (blockheader.GetBlockTime() > params.lwma1Timestamp) {
		if(cache) return CheckProofOfWorkSoterC(blockheader.GetHash(), blockheader.nBits, params, blockheader.GetPoWType());
		else if(!cache) return CheckProofOfWorkSoterC(blockheader.GetHash(false), blockheader.nBits, params, blockheader.GetPoWType());
    }
	else {
        if (cache) return CheckProofOfWork(blockheader.GetHash(), blockheader.nBits, params);
        else if(!cache) return CheckProofOfWork(blockheader.GetHash(false), blockheader.nBits, params);
    }
}
