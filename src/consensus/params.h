// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-present The Soteria Core developers

#ifndef SOTERIA_CONSENSUS_PARAMS_H
#define SOTERIA_CONSENSUS_PARAMS_H

#include "founder_payment.h"
#include <limits>       // For std::numeric_limits
#include <cstdint>      // For uint32_t/uint64_t
#include <map>
#include <string>
#include <uint256.h>

namespace Consensus 
{

/**
 * BIP9 deployments
 */
enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    // DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    // DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp,
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Soteria network upgrades using timestamps
 */
enum UpgradeIndex {
    SOTERG_SWITCH,
    SOTERC_SWITCH,
    SOTERIA_ASSETS,
    SOTERIA_SMART_CONTRACTS,
    SOTERIA_NAME_SYSTEM,
    MAX_NETWORK_UPGRADES
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
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
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;

    /** BIP9 deployments */
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];

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
    uint32_t nSoterGTimestamp;

    // Consensus fields
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
    int64_t lwmaAveragingWindow;        // Averaging window size for LWMA-EMA3 diff adjust
    std::vector<uint256> powTypeLimits; // Limits for each pow type (with future-proofing space; can't pick up NUM_BLOCK_TYPES here)
    
};
} // namespace Consensus

#endif // SOTERIA_CONSENSUS_PARAMS_H
