// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_CONSENSUS_PARAMS_H
#define SOTERIA_CONSENSUS_PARAMS_H

#include <uint256.h>
#include "founder_payment.h"
#include <limits>       // For std::numeric_limits
#include <cstdint>      // For uint32_t/uint64_t
#include <map>
#include <string>
namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp,
   DEPLOYMENT_PQ_HYBRID, // Deployment of RIP-25: Post-Quantum Hybrid Signatures (ML-DSA-44)
   MAX_VERSION_BITS_DEPLOYMENTS
};

enum UpgradeIndex {
    SOTERG_SWITCH,
    SOTERC_SWITCH,
    SOTERIA_ASSETS,
    SOTERIA_SMART_CONTRACTS,
    SOTERIA_NAME_SYSTEM,
    MAX_NETWORK_UPGRADES
};

struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
    /** Use to override the confirmation window on a specific BIP */
    uint32_t nOverrideMinerConfirmationWindow;
    /** Use to override the the activation threshold on a specific BIP */
    uint32_t nOverrideRuleChangeActivationThreshold;
};

struct NetworkUpgrade {
    uint32_t nTimestamp;
};
/**
 * Parameters that influence chain consensus.
 */
struct ConsensusParams {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /** Block height and hash at which BIP34 becomes active */
    bool nBIP34Enabled;
    bool nBIP65Enabled;
    bool nBIP66Enabled;
    int BIP34LockedIn;
    // uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    // int BIP65Height;
    /** Block height at which BIP66 becomes active */
    // int BIP66Height;

    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;

    /** BIP9 deployments */
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];

    /** Soteria network upgrades */
    NetworkUpgrade vUpgrades[MAX_NETWORK_UPGRADES];

    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    uint64_t nBlockTimeDivisor;
    uint64_t nOutboundCycleSeconds;
    uint32_t nEndCycleMarginPct;
    uint32_t nNearBoundaryExtraBlocks;
    uint32_t nBurstWindowSeconds;
    uint32_t nBurstFactorTenths;
    int64_t nPowTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;
    bool nSegwitEnabled;
    bool nCSVEnabled;
    bool nPQHybridEnabled; // RIP-25: Post-Quantum Hybrid Signatures
    uint32_t nSoterGTimestamp;

    // Dual Algo consensus fields
    int64_t lwmaTimestamp;
    int64_t lwma1Timestamp;
    int64_t lwmaHWCA;
    int64_t lwmaHeight;

    int64_t diffRetargetStartHeight; 
    int64_t diffRetargetEndHeight;   
    int64_t diffRetargetStartHeight1; 
    int64_t diffRetargetEndHeight1;   
    int64_t diffRetargetStartHeight2; 
    int64_t diffRetargetEndHeight2;   
    int64_t diffRetargetStartHeight3; 
    int64_t diffRetargetEndHeight3;   
    int64_t diffRetargetStartHeight4; 
    int64_t diffRetargetEndHeight4;   
    int64_t diffRetargetStartHeight5; 
    int64_t diffRetargetEndHeight5;   
    int64_t diffRetargetStartHeight6; 
    int64_t diffRetargetEndHeight6;
    
    int64_t lwmaAveragingWindow;        // Averaging window size for LWMA-EMA3 diff adjust
    std::vector<uint256> powTypeLimits; // Limits for each pow type (with future-proofing space; can't pick up NUM_BLOCK_TYPES here)
    
};
} // namespace Consensus

#endif
