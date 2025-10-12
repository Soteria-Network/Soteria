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
    int64_t nPastBlocks = 360;

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

/*    // Limit "High Hash" Attacks... Progressively lower mining difficulty if too high...
    if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 30){
	//LogPrintf("DarkGravityWave: 30 minutes without a block !! Resetting difficulty ! OLD Target = %s\n", bnNew.ToString());
	bnNew = bnPowLimit;
	//LogPrintf("DarkGravityWave: 30 minutes without a block !! Resetting difficulty ! NEW Target = %s\n", bnNew.ToString());
    }

    else if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 25){
	//LogPrintf("DarkGravityWave: 25 minutes without a block. OLD Target = %s\n", bnNew.ToString());
	bnNew *= 100000;
	//LogPrintf("DarkGravityWave: 25 minutes without a block. NEW Target = %s\n", bnNew.ToString());
    }

    else if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 20){
	//LogPrintf("DarkGravityWave: 20 minutes without a block. OLD Target = %s\n", bnNew.ToString());
	bnNew *= 10000;
	//LogPrintf("DarkGravityWave: 20 minutes without a block. NEW Target = %s\n", bnNew.ToString());
    }

    else if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 15){
	//LogPrintf("DarkGravityWave: 15 minutes without a block. OLD Target = %s\n", bnNew.ToString());
	bnNew *= 1000;
	//LogPrintf("DarkGravityWave: 15 minutes without a block. NEW Target = %s\n", bnNew.ToString());
    }

    else if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 10){
	//LogPrintf("DarkGravityWave: 10 minutes without a block. OLD Target = %s\n", bnNew.ToString());
	bnNew *= 100;
	//LogPrintf("DarkGravityWave: 10 minutes without a block. NEW Target = %s\n", bnNew.ToString());
    }

    else {
	bnNew = bnNew;
	//LogPrintf("DarkGravityWave: no stale tip over 10m detected yet so target = %s\n", bnNew.ToString());
    }
*/

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
        
    int64_t dgwWindow = 0; // does not have DGWPastBlocks so no need to check.

    const CBlockIndex* pindex = pindexLast;
    
    while (pindex->pprev && dgwWindow > 0) {
        pindex = pindex->pprev;
        dgwWindow--;
    }
    
    return pindex->nTime <= params.nSoterGTimestamp;
}

// Copyright (c) 2017-2021 The Bitcoin Gold developers, Zawy, iamstenman (Microbitcoin), The Litecoin Cash developers, The Soteria developers
// MIT License
// Algorithm by Zawy, a modification of WT-144 by Tom Harding
// For updates see
// https://github.com/zawy12/difficulty-algorithms/issues/3#issuecomment-442129791

unsigned int GetNextWorkRequiredLWMA(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
    if (!pindexLast || pindexLast->nHeight >= params.lwmaHeight || pindexLast->GetBlockTime() >= params.lwmaTimestamp)
        return GetNextWorkRequiredLWMA1(pindexLast, pblock, params, powType);
    else if (pindexLast->nHeight >= params.lwma1Height && pindexLast->GetBlockTime() < params.lwma1Timestamp)
        return GetNextWorkRequiredLWMA2(pindexLast, pblock, params, powType);  
    else
        return GetNextWorkRequiredLWMA3(pindexLast, pblock, params, powType);
}

unsigned int GetNextWorkRequiredLWMA1(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::ConsensusParams& params,
    const POW_TYPE powType)
{
    // Input validation
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA1\n");
        return UintToArith256(params.powTypeLimits[powType]).GetCompact();
    }

    // Chain continuity
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain at height %d\n", pindexLast->nHeight);
        return UintToArith256(params.powTypeLimits[powType]).GetCompact();
    }

    // Logging throttle per POW_TYPE
 // const size_t TYPE_COUNT = sizeof(POW_TYPE_NAMES) / sizeof(POW_TYPE_NAMES[0]);
    const size_t TYPE_COUNT = std::size(POW_TYPE_NAMES);
    static std::array<int64_t, TYPE_COUNT> lastLogTime = {};
    bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    int64_t now = GetTime();
    bool logThis = verbose && (now - lastLogTime[static_cast<size_t>(powType)] > 10);
    if (logThis) lastLogTime[static_cast<size_t>(powType)] = now;

    // Parameters
	UintToArith256(params.powTypeLimits[powType]);
    const int64_t spacingPerAlgo = params.nPowTargetSpacing * 2;
    const int64_t T = spacingPerAlgo;
    const int64_t N = params.lwmaAveragingWindow;
    const __int128 k = (__int128(N) * (N + 1) / 2) * T;             // weighted sum timespan
    int64_t height = pindexLast->nHeight;

    // Early exit on short chain
    if (height < N + 1) {
        if (logThis) {
            LogPrintf("* Short chain [%d < %d], using powLimit\n", height, N+1);
        }
        return UintToArith256(params.powTypeLimits[powType]).GetCompact();
    }
    // Early exit if we have no last block
    if (!pindexLast) {
        return UintToArith256(params.powTypeLimits[powType]).GetCompact();
    }

    // Collect last N+1 same-type blocks
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(N + 1);

    const int64_t lwmaTime = params.lwmaTimestamp;
    const CBlockIndex* walker = pindexLast;
    int scanned = 0;
    const int maxDepth = 30;
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
            LogPrintf("* Found only %zu/%d blocks, using powLimit\n",
                      blocks.size(), N+1);
        }
        return UintToArith256(params.powTypeLimits[powType]).GetCompact();
    }

    // Use 128-bit accumulator for weighted solve times, Compute LWMA
    __int128 totalWeightedSolveTime = 0;
    arith_uint256 totalTarget = 0;
    int64_t prevTime = blocks.back()->GetBlockTime();

    for (int i = 0; i < N; ++i) {
        const auto* b = blocks[N - 1 - i];  // newest first
        int64_t dt = b->GetBlockTime() - prevTime;
        prevTime = b->GetBlockTime();

        // Bound solve time to [1, 4*T], old practice 6, new 4
        dt = std::max<int64_t>(1, std::min<int64_t>(dt, 3 * T));
        int weight = i + 1;  // 1 for oldest, N for newest

        int64_t wdt;
        if (!SafeMultiply(dt, weight, wdt)) {
            if (logThis) {
                LogPrintf("ERROR: Overflow solving weight@height %d\n", b->nHeight);
            }
            return UintToArith256(params.powTypeLimits[powType]).GetCompact();
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

    // Clamp to [minTarget, powTypeLimits]
    arith_uint256 minTarget = UintToArith256(params.powTypeLimits[powType]) >> 3;  // no harder than 4× the limit as in dt
    if (nextTarget < minTarget) {
        if (logThis) LogPrintf("* Raising target to lower clamp\n");
        nextTarget = minTarget;
    }
    if (nextTarget > UintToArith256(params.powTypeLimits[powType])) {
        if (logThis) LogPrintf("* Clamping target to powLimit\n");
        nextTarget = UintToArith256(params.powTypeLimits[powType]);
    }

    // Last block's compact target
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    // EWA overlay to smooth random noise, α = 1/8 => newWeight = 1, oldWeight = 7
    constexpr uint64_t EWMA_NUM = 1, EWMA_DEN = 8;
    arith_uint256 smoothed = (
        nextTarget * EWMA_NUM +
        lastTarget * (EWMA_DEN - EWMA_NUM)
    ) / EWMA_DEN;

    if (smoothed > UintToArith256(params.powTypeLimits[powType])) {
        smoothed = UintToArith256(params.powTypeLimits[powType]);
    }

    if (logThis) {
        LogPrintf("* lwma_target=%s ewa_smoothed=%s\n",
                  nextTarget.ToString(), smoothed.ToString());
    }

    return smoothed.GetCompact();
}

/* only in Testnet
// Safe multiplication helper for int64_t
namespace {
    bool SafeMultiply(int64_t a, int64_t b, int64_t& result) {
        // Handle zero cases first
        if (a == 0 || b == 0) {
            result = 0;
            return true;
        }
        
        // Positive × Positive
        if (a > 0 && b > 0) {
            if (a > std::numeric_limits<int64_t>::max() / b) return false;
        }
        // Negative × Negative
        else if (a < 0 && b < 0) {
            if (a < std::numeric_limits<int64_t>::max() / b) return false;
        }
        // Mixed signs
        else {
            if (b > 0) {
                if (a < std::numeric_limits<int64_t>::min() / b) return false;
            } else {
                if (b < std::numeric_limits<int64_t>::min() / a) return false;
            }
        }
        
        result = a * b;
        return true;
    }
}

unsigned int GetNextWorkRequiredLWMA1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType) {
    // ===== Input Validation =====
    if (!pindexLast || !pblock) {
        LogPrintf("ERROR: Null input in GetNextWorkRequiredLWMA1\n");
        return UintToArith256(params.powLimit).GetCompact(); 
    }
    
    // ===== Chain Continuity Check =====
    if (pindexLast->nHeight > 0 && !pindexLast->pprev) {
        LogPrintf("ERROR: Broken chain in GetNextWorkRequiredLWMA1 (height %d)\n", pindexLast->nHeight);
        return UintToArith256(params.powLimit).GetCompact();
    }

    const bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    const int64_t T = params.nPowTargetSpacing * 2;  // 30 seconds for dual-algo
    const int64_t N = params.lwmaAveragingWindow;
    const int64_t k = (N * (N + 1)) / 2 * T;  // Sum of weights * target time (488,700)
    const int64_t height = pindexLast->nHeight;
    
    // ===== Rate Limiting for Logs =====
    static std::map<POW_TYPE, int64_t> lastLogTime;
    const int64_t now = GetTime();
    const bool logThisCall = verbose && (now - lastLogTime[powType] > 30);
    if (logThisCall) lastLogTime[powType] = now;

    // Need N+1 blocks for N solve times
    if (height < N + 1) {
        if (logThisCall) LogPrintf("* Allowing %s pow limit (short chain, height=%d < %d)\n", 
                                  POW_TYPE_NAMES[powType], height, N+1);
        return powLimit.GetCompact();
    }

    // Collect N+1 consecutive blocks of this POW type
    std::vector<const CBlockIndex*> blocks;
    const CBlockIndex* blockWalker = pindexLast;
    int collected = 0;
    
    while (collected < (N + 1) && blockWalker) {
        // Skip pre-fork blocks
        if (blockWalker->GetBlockHeader().nTime < params.lwmaTimestamp) {
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
        return powLimit.GetCompact();
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
            return powLimit.GetCompact();
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
    // Only clamp upper bound (powLimit)
    if (nextTarget > powLimit) {
        if (logThisCall) LogPrintf("* Clamping %s target to limit\n", POW_TYPE_NAMES[powType]);
        return powLimit.GetCompact();
    }

    return nextTarget.GetCompact();
}
*/
unsigned int GetNextWorkRequiredLWMA2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
    const bool verbose = LogAcceptCategory(BCLog::SOTERC_SWITCH);
    const int64_t N = 600; // def=45  params.lwmaAveragingWindow              // Window size
    const int64_t k = 8925; // def=1277≈1276.5,                               // Constant for proper averaging after weighting solvetimes (k=(N+1)/2*TargetSolvetime*0.998), 
    const arith_uint256 powLimit = UintToArith256(params.powLimit);           // Minimum diff, 00000ffff... for both algos
    const int height = pindexLast->nHeight + 1;                               // Block height
    assert(height > N);

    // TESTNET ONLY: Allow minimum difficulty blocks if we haven't seen a block for ostensibly 10 blocks worth of time.
    // ***** THIS IS NOT SAFE TO DO ON YOUR MAINNET! *****
    if (params.fPowAllowMinDifficultyBlocks &&
        pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA2: Allowing %s pow limit (apparent testnet stall)\n", POW_TYPE_NAMES[powType]);
        return powLimit.GetCompact();
    }

    if (params.fPowNoRetargeting) {
        return pindexLast->nBits;
    }

    arith_uint256 sum_target;
    int t = 0, j = 0, blocksFound = 0;
    
    // Loop through N most recent blocks.
    for (int i = height - N; i < height; i++) {

        if (pindexLast->GetBlockHeader().GetPoWType() != powType) {
            if (verbose) LogPrintf("* GetNextWorkRequiredLWMA2: Height %i: Skipping %s (wrong blocktype)\n", pindexLast->nHeight, pindexLast->GetBlockHeader().GetHash().ToString().c_str());
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

        // Target sum divided by a factor, (k N^2).
        // The factor is a part of the final equation. However we divide sum_target here to avoid
        // potential overflow.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sum_target += target / (k * N * N);
    }


    // Keep t reasonable in case strange solvetimes occurred.
    if (t < N * k / 3) {
        t = N * k / 3;
    }


    arith_uint256 next_target = t * sum_target;
    if (next_target > powLimit) {
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA2: Allowing %s pow limit (target too high)\n", POW_TYPE_NAMES[powType]);
        next_target = powLimit;
    }

    // If no blocks of correct POW_TYPE was found within the block window then return powLimit
    if(blocksFound == 0){
        if (verbose) LogPrintf("* GetNextWorkRequiredLWMA2: Allowing %s pow limit (blocksFound returned 0)\n", POW_TYPE_NAMES[powType]);
        return powLimit.GetCompact();
    }

    return next_target.GetCompact();
}

unsigned int GetNextWorkRequiredLWMA3(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::ConsensusParams& params, const POW_TYPE powType)
{
	// Originally from XVG repository

	// LWMA for BTC clones
	// Algorithm by zawy, LWMA idea by Tom Harding
	// Code by h4x3rotab of BTC Gold, modified/updated by zawy
	// https://github.com/zawy12/difficulty-algorithms/issues/3#issuecomment-388386175
	//  FTL must be changed to about N*T/20 = 360 for T=120 and N=60 coins.
	//  FTL is MAX_FUTURE_BLOCK_TIME in chain.h.
	//  FTL in Ignition, Numus, and others can be found in main.h as DRIFT.
	//  Some coins took out a variable, and need to change the 2*60*60 here:
	//  if (block.GetBlockTime() > nAdjustedTime + 2 * 60 * 60)

	const arith_uint256 powLimit = UintToArith256(params.powTypeLimits[powType]);  // Minimum diff, 00000ffff... for soterg, 000fffff... for soterc
	const int64_t T = params.nPowTargetSpacing * 2;                                // Target freq 15s x 2 algos
	const int64_t N = 600; // def=60   params.lwmaAveragingWindow                  // Window size - 60 as per zawy12 graphs
	const int64_t k = 8997; // ≈2%                                                 // Constant for proper averaging after weighting solvetimes == 109800, N * (N + 1) * T / 2
	const int64_t height = pindexLast->nHeight + 1;                                // Block height, last + 1, since we are finding next block diff

	arith_uint256 sum_target;
	int64_t t = 0, j = 0;
	int64_t solvetime = 0;

	std::vector<const CBlockIndex*> SameAlgoBlocks;
	for (int c = height-1; SameAlgoBlocks.size() < (N + 1); c--){
		const CBlockIndex* block = pindexLast->GetAncestor(c); // -1 after execution
		if (block->GetBlockHeader().GetPoWType() == powType){
			SameAlgoBlocks.push_back(block);
		}

		if (c < 100){ // If there are not enough blocks with this algo, return powLimit until dataset is big enough
			return powLimit.GetCompact();
		}
	}
	// Creates vector with {block1000, block997, block993}, so we start at the back

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

		// Target sum divided by a factor, (k N^2).
		// The factor is a part of the final equation. However we divide sum_target here to avoid
		// potential overflow.
		arith_uint256 target;
		target.SetCompact(block->nBits);
		sum_target += target / N / k;
	}

	arith_uint256 next_target = t * sum_target;

	if (next_target > powLimit) {
		next_target = powLimit;
	}

	return next_target.GetCompact();
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
	if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powTypeLimits[POW_TYPE_SOTERC]))
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
