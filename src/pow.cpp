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
                       
    // Fallback to default LWMA1 to keep blocks valid
    return GetNextWorkRequiredLWMA1(pindexLast, pblock, params, powType); 
}

// Height, LWMA 60, dt 3, pt 2, ewma 1/2, first 360 blocks
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
    const int64_t N = 60;
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

        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T));
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
    arith_uint256 minTarget = powTypeLimit >> 2; // for first blocks

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
    
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 2;
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

// default
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
    const int64_t N = 60;
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

        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 6 * T)); // def=4 , 2 is too strict, 4 standard, 3 balanced, min=2.5
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
    arith_uint256 minTarget = powTypeLimit >> 3;
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
    constexpr uint64_t EWMA_NUM = 3, EWMA_DEN = 4;
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

// Height1, LWMA2 60, dt 4, pt 2, ewma 3/4
unsigned int GetNextWorkRequiredLWMA2(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 60);
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);
    const int64_t T = params.nPowTargetSpacing;        
    const int64_t N = 60;                              
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;
    const int64_t height = pindexLast->nHeight + 1;


    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA2\n");
        return powTypeLimit.GetCompact();
    }
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return powTypeLimit.GetCompact();
    }

    if (height < N + 1) {
        if (logThis) LogPrintf("* Short chain [%d < %d], using powTypeLimit\n", height, N+1);
        return powTypeLimit.GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 300;  // safety
    while (walker && blocks.size() < N + 1 && scanned < maxDepth) {
        if (walker->GetBlockHeader().GetPoWType() == powType) {
            blocks.push_back(walker);
        }
        if (walker->nHeight <= params.lwmaHWCA - int(N)) break;
        walker = walker->pprev;
        ++scanned;
    }
    if (blocks.size() < N + 1) {
        if (logThis) LogPrintf("* Found only %zu/%d blocks, using powTypeLimit\n", blocks.size(), N+1);
        return powTypeLimit.GetCompact();
    }

    // LWMA with bounded dt and 128-bit accumulator
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;

    // prevTime = oldest timestamp (index back())
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i]; // from second-oldest to newest
        int64_t rawDt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [0.33*T, 6*T]
        // Prevent tiny dt (e.g., 0.5 s) and extreme outliers from skewing averages
        int64_t dt = rawDt;
        if (dt < (T / 3)) dt = (T / 3);      // ≈ 4 s at T=12 s
        if (dt > (6 * T)) dt = (6 * T);      // 72 s cap

        int weight = i + 1; // 1 for oldest in window, N for newest
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        arith_uint256 tgt;
        tgt.SetCompact(b->nBits);
        totalTarget += tgt;
    }

    // Average target of past N blocks (same type)
    arith_uint256 avgTarget = totalTarget / N;

    // Compute nextTarget = round( avgTarget * (weightedTime / k) )
    arith_uint256 weightedTime = arith_uint256(totalWeightedSolveTime);
    arith_uint256 K            = arith_uint256(k);
    arith_uint256 numerator    = avgTarget * weightedTime;
    arith_uint256 halfK        = K >> 1;
    arith_uint256 nextTarget   = (numerator + halfK) / K;

    // Clamp to [minTarget, powTypeLimit]
    arith_uint256 minTarget = powTypeLimit >> 3; // floor at 8× harder than limit
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

    // Per-block ratio clamp relative to lastTarget to avoid whiplash, but allow fast correction
    // factor = nextTarget / lastTarget (targets: bigger = easier, smaller = harder)
    // Allow factor in [0.7, 1.35] during stabilization
    arith_uint256 factorNum = nextTarget;
    arith_uint256 factorDen = lastTarget;
    // Compute factor in fixed-point via scaled division (approximate clamp)
    // We can clamp by reconstructing candidate bounds:
    arith_uint256 downBound = lastTarget; downBound = (downBound * 7) / 10;   // 0.7×
    arith_uint256 upBound   = lastTarget; upBound   = (upBound   * 135) / 100; // 1.35×

    if (nextTarget < downBound) nextTarget = downBound;
    if (nextTarget > upBound)   nextTarget = upBound;

    // EWMA overlay: 75% new, 25% old
    constexpr uint64_t EWMA_NUM = 3, EWMA_DEN = 4;
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > powTypeLimit) smoothed = powTypeLimit;
    if (smoothed < minTarget)     smoothed = minTarget;

    if (logThis) {
        LogPrintf("* lwma_target=%s clamped=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

// Height2
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
 
unsigned int GetNextWorkRequiredLWMA4(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{

    // ===== Parameters =====
    const int64_t T = 12;             // target block time (seconds)
    const int64_t N = 60;             // LWMA window
    const int64_t BOOTSTRAP_HEIGHT = 1440;

    const arith_uint256 powTypeLimit = UintToArith256(params.powTypeLimits[powType]);

    // ===== Basic guards =====
    if (!pindexLast || !pblock) {
        return powTypeLimit.GetCompact();
    }
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        return powTypeLimit.GetCompact();
    }

    const int64_t height = pindexLast->nHeight + 1;
    const bool BOOTSTRAP = (height < BOOTSTRAP_HEIGHT);

    // k = T * (1 + 2 + ... + N) = T * N*(N+1)/2
    const uint64_t k = static_cast<uint64_t>((static_cast<uint64_t>(N) * (N + 1) / 2) * T);

    // ===== Bootstrap when chain is short =====
    if (height < N + 1) {
        return powTypeLimit.GetCompact();
    }

    // ===== Collect last N+1 contiguous blocks (single PoW algorithm) =====
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(static_cast<size_t>(N) + 1);

    const CBlockIndex* walker = pindexLast;
    while (walker && blocks.size() < static_cast<size_t>(N) + 1) {
        blocks.push_back(walker);      // blocks[0] = newest, blocks[N] = oldest
        walker = walker->pprev;
    }

    // Safety: if we didn't collect enough blocks (shouldn't happen due to height guard)
    if (blocks.size() < static_cast<size_t>(N) + 1) {
        if (BOOTSTRAP) return powTypeLimit.GetCompact();
        // Fallback: blend last target with limit
        arith_uint256 lastSame; lastSame.SetCompact(pindexLast->nBits);
        arith_uint256 fallback = (lastSame * 3 + powTypeLimit) / 4; // 75% last, 25% limit
        return fallback.GetCompact();
    }

    // ===== LWMA core with bounded dt =====
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;

    // Phase-dependent dt bounds
    const int64_t dtLower = std::max<int64_t>(1, T / 3);
    const int64_t dtUpper = BOOTSTRAP ? (4 * T) : (3 * T);

    // Minimal patch: iterate adjacent pairs in a single, consistent order (newest -> older)
    // blocks[0] newest, blocks[1] next, ... blocks[N] oldest
    for (int i = 0; i < N; ++i) {
        const int idx_newer = i;
        const int idx_older = i + 1;
        
    // Compute raw interval
    int64_t rawDt = blocks[idx_newer]->GetBlockTime() - blocks[idx_older]->GetBlockTime();
    if (rawDt <= 0) { rawDt = dtLower; }
        int64_t dt = rawDt;
        if (dt < dtLower) dt = dtLower;
        if (dt > dtUpper) dt = dtUpper;

        // Weight more recent intervals higher
        const int weight = N - i;
        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            return powTypeLimit.GetCompact();
        }
        totalWeightedSolveTime += wdt;

        // Average targets over the same N blocks (align with intervals)
        arith_uint256 tgt; tgt.SetCompact(blocks[idx_newer]->nBits);
        totalTarget += tgt;
    }

    // Average target and next target (rounded)
    arith_uint256 avgTarget = totalTarget / N;

    // Safe conversion of weighted solve time to 256-bit
    const uint64_t sumWeightedDt64 = static_cast<uint64_t>(totalWeightedSolveTime);
    arith_uint256 weightedTime = arith_uint256(sumWeightedDt64);

    arith_uint256 K = arith_uint256(k);
    arith_uint256 numerator = avgTarget * weightedTime;
    arith_uint256 halfK = K >> 1;
    arith_uint256 nextTarget = (numerator + halfK) / K;

    // ===== Phase-dependent floor and limit clamps =====
    // NOTE: right-shift reduces numeric target (harder). Tuned shifts below.
    const unsigned minTargetShiftBootstrap = 2; // >>2 (1/4 numeric target)
    const unsigned minTargetShiftNormal    = 3; // >>3 (1/8 numeric target)
    arith_uint256 minTarget = BOOTSTRAP ? (powTypeLimit >> minTargetShiftBootstrap)
                                        : (powTypeLimit >> minTargetShiftNormal);

    if (nextTarget < minTarget)     nextTarget = minTarget;
    if (nextTarget > powTypeLimit)  nextTarget = powTypeLimit;

    // ===== Use last target (single-algo) =====
    arith_uint256 lastTarget; lastTarget.SetCompact(pindexLast->nBits);

    // ===== Per-block ratio clamp (phase-dependent) =====
    // Smaller target => harder difficulty; larger target => easier.
    arith_uint256 downBound, upBound;
    if (BOOTSTRAP) {
        // Aggressive correction to escape fast/slow bursts
        downBound = (lastTarget * 60) / 100;   // −40%
        upBound   = (lastTarget * 150) / 100;  // +50%
    } else {
        // Stability-first to reduce overshoot and oscillations
        downBound = (lastTarget * 75) / 100;   // −25%
        upBound   = (lastTarget * 130) / 100;  // +30%
    }

    if (nextTarget < downBound) nextTarget = downBound;
    if (nextTarget > upBound)   nextTarget = upBound;

    // ===== EWMA smoothing (phase-dependent) =====
    // BOOTSTRAP: 1/2 (visible adjustment, quicker response)
    // STRICT:    1/3 (smoother, reduced jitter)
    arith_uint256 smoothed;
    if (BOOTSTRAP) {
        constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 2;
        smoothed = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
    } else {
        constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 3;
        smoothed = (nextTarget * EWMA_NUM + lastTarget * (EWMA_DEN - EWMA_NUM)) / EWMA_DEN;
    }

    // Final safety clamps
    if (smoothed > powTypeLimit) smoothed = powTypeLimit;
    if (smoothed < minTarget)    smoothed = minTarget;

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
