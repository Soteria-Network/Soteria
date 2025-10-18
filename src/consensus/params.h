// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_CONSENSUS_PARAMS_H
#define SOTERIA_CONSENSUS_PARAMS_H

#include <uint256.h>
#include "founder_payment.h"
#include <limits>       // For std::numeric_limits
#include <cstdint>      // For uint32_t/uint64_t
#include <map>
#include <string>
namespace Consensus {

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
/** Need it for testing purposes
    /** Constant for nTimeout very far in the future. 
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. 
    static constexpr int64_t ALWAYS_ACTIVE = -1; 
*/

/**
    // Use to override the confirmation window on a specific BIP 
    uint32_t nOverrideMinerConfirmationWindow;
    // Use to override the the activation threshold on a specific BIP 
    uint32_t nOverrideRuleChangeActivationThreshold;
*/
/**
 * Struct for each network upgrade using timestamp.
 */
// Consider narrowing nTimestamp to uint32_t if possible (sufficient until 2106)
struct NetworkUpgrade {
    uint32_t nTimestamp;
};
/**
 * Parameters that influence chain consensus.
 */
struct ConsensusParams {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;   // TODO: eliminate this after adjusting the unit tests
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
    uint32_t nSoterGTimestamp;

    // Dual Algo consensus fields
    int64_t lwmaTimestamp;
    int64_t lwma1Timestamp;
    int64_t lwmaHWCA;
    int64_t lwmaHeight;

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
    int64_t diffRetargetStartHeight7; 
    int64_t diffRetargetEndHeight7;   
    int64_t diffRetargetStartHeight8; 
    int64_t diffRetargetEndHeight8;          
    int64_t diffRetargetStartHeight9; 
    int64_t diffRetargetEndHeight9;   
    int64_t diffRetargetStartHeight10; 
    int64_t diffRetargetEndHeight10;   
    int64_t diffRetargetStartHeight11; 
    int64_t diffRetargetEndHeight11;  
    int64_t diffRetargetStartHeight12; 
    int64_t diffRetargetEndHeight12;
    int64_t diffRetargetStartHeight13; 
    int64_t diffRetargetEndHeight13;
    int64_t diffRetargetStartHeight14; 
    int64_t diffRetargetEndHeight14;  
    int64_t diffRetargetStartHeight15; 
    int64_t diffRetargetEndHeight15;
    int64_t diffRetargetStartHeight16; 
    int64_t diffRetargetEndHeight16;
    int64_t diffRetargetStartHeight17; 
    int64_t diffRetargetEndHeight17;
    int64_t diffRetargetStartHeight18; 
    int64_t diffRetargetEndHeight18;
    int64_t diffRetargetStartHeight19; 
    int64_t diffRetargetEndHeight19;
    int64_t diffRetargetStartHeight20; 
    int64_t diffRetargetEndHeight20;
    int64_t diffRetargetStartHeight21; 
    int64_t diffRetargetEndHeight21;
    int64_t diffRetargetStartHeight22; 
    int64_t diffRetargetEndHeight22;
    int64_t diffRetargetStartHeight23; 
    int64_t diffRetargetEndHeight23;
    int64_t diffRetargetStartHeight24; 
    int64_t diffRetargetEndHeight24;
    int64_t diffRetargetStartHeight25; 
    int64_t diffRetargetEndHeight25;
    int64_t diffRetargetStartHeight26; 
    int64_t diffRetargetEndHeight26;
    int64_t diffRetargetStartHeight27; 
    int64_t diffRetargetEndHeight27;
    int64_t diffRetargetStartHeight28; 
    int64_t diffRetargetEndHeight28;
    int64_t diffRetargetStartHeight29; 
    int64_t diffRetargetEndHeight29;
    int64_t diffRetargetStartHeight30; 
    int64_t diffRetargetEndHeight30;
    int64_t diffRetargetStartHeight31; 
    int64_t diffRetargetEndHeight31;
    int64_t diffRetargetStartHeight32; 
    int64_t diffRetargetEndHeight32;
    int64_t diffRetargetStartHeight33; 
    int64_t diffRetargetEndHeight33;
    int64_t diffRetargetStartHeight34; 
    int64_t diffRetargetEndHeight34;
    int64_t diffRetargetStartHeight35; 
    int64_t diffRetargetEndHeight35;
    int64_t diffRetargetStartHeight36; 
    int64_t diffRetargetEndHeight36;
    int64_t diffRetargetStartHeight37; 
    int64_t diffRetargetEndHeight37;
    int64_t diffRetargetStartHeight38; 
    int64_t diffRetargetEndHeight38;
    int64_t diffRetargetStartHeight39; 
    int64_t diffRetargetEndHeight39;
    int64_t lwmaAveragingWindow;        // Averaging window size for LWMA-EMA3 diff adjust
    std::vector<uint256> powTypeLimits; // Limits for each pow type (with future-proofing space; can't pick up NUM_BLOCK_TYPES here)
    
};
} // namespace Consensus

#endif // SOTERIA_CONSENSUS_PARAMS_H
