// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_CHAINPARAMS_H
#define SOTERIA_CHAINPARAMS_H

#include <chainparamsbase.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <protocol.h>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <chain.h>
struct CDNSSeedData {
    std::string host;
    bool supportsServiceBitsFiltering;
    CDNSSeedData(const std::string &strHost, bool supportsServiceBitsFilteringIn) : host(strHost), supportsServiceBitsFiltering(supportsServiceBitsFilteringIn) {}
};

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;
};

struct ChainTxData {
    int64_t nTime;
    int64_t nTxCount;
    double dTxRate;
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Soteria system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,
        MAX_BASE58_TYPES
    };

    const Consensus::ConsensusParams& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    constexpr int GetDefaultPort() const { return nDefaultPort; }
    bool MiningRequiresPeers() const {return fMiningRequiresPeers; }
    const CBlock& GenesisBlock() const { return genesis; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Policy: Filter transactions that do not match well-defined patterns */
    bool RequireStandard() const { return fRequireStandard; }
    constexpr uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool MineBlocksOnDemand() const { return fMineBlocksOnDemand; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    constexpr int ExtCoinType() const { return nExtCoinType; }
    const std::vector<SeedSpec6>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    const ChainTxData& TxData() const { return chainTxData; }
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout);
    void TurnOffSegwit();
    void TurnOffCSV();
    void TurnOffBIP34();
    void TurnOffBIP65();
    void TurnOffBIP66();
    bool BIP34();
    bool BIP65();
    bool BIP66();
    bool CSVEnabled() const;

    /** SOTER Start **/
    const CAmount& IssueAssetBurnAmount() const { return nIssueAssetBurnAmount; }
    const CAmount& ReissueAssetBurnAmount() const { return nReissueAssetBurnAmount; }
    const CAmount& IssueSubAssetBurnAmount() const { return nIssueSubAssetBurnAmount; }
    const CAmount& IssueUniqueAssetBurnAmount() const { return nIssueUniqueAssetBurnAmount; }
    const CAmount& IssueMsgChannelAssetBurnAmount() const { return nIssueMsgChannelAssetBurnAmount; }
    const CAmount& IssueQualifierAssetBurnAmount() const { return nIssueQualifierAssetBurnAmount; }
    const CAmount& IssueSubQualifierAssetBurnAmount() const { return nIssueSubQualifierAssetBurnAmount; }
    const CAmount& IssueRestrictedAssetBurnAmount() const { return nIssueRestrictedAssetBurnAmount; }
    const CAmount& AddNullQualifierTagBurnAmount() const { return nAddNullQualifierTagBurnAmount; }

    // Funds
    const CAmount& MiningFund() const { return nMiningFund; }
    const CAmount& NodeOperatorsFund() const { return nNodeOperatorsFund; }
//    const CAmount& AmbassadorFund() const { return nAmbassadorFund; }
    const CAmount& StakingPool() const { return nStakingPool; }
    const CAmount& ExchangeLiquidityFund() const { return nExchangeLiquidityFund; }
    const CAmount& BackersFund() const { return nBackersFund; }
    const CAmount& CompensationFund() const { return nCompensationFund; }
    const CAmount& CommunityFund() const { return nCommunityFund; }
    const CAmount& EcosystemGrowthFund() const { return nEcosystemGrowthFund; }
    const CAmount& DevTeamFund() const { return nDevTeamFund; }
    const CAmount& SnapshotFund() const { return nSnapshotFund; }
    const CAmount& MarketingFund() const { return nMarketingFund; }
    const CAmount& FoundationReserveFund() const { return nFoundationReserveFund; }
    const CAmount& ContributorsFund() const { return nContributorsFund; }
    const CAmount& GlobalBurnFund() const { return nGlobalBurnFund; }
    

    const std::string& IssueAssetBurnAddress() const { return strIssueAssetBurnAddress; }
    const std::string& ReissueAssetBurnAddress() const { return strReissueAssetBurnAddress; }
    const std::string& IssueSubAssetBurnAddress() const { return strIssueSubAssetBurnAddress; }
    const std::string& IssueUniqueAssetBurnAddress() const { return strIssueUniqueAssetBurnAddress; }
    const std::string& IssueMsgChannelAssetBurnAddress() const { return strIssueMsgChannelAssetBurnAddress; }
    const std::string& IssueQualifierAssetBurnAddress() const { return strIssueQualifierAssetBurnAddress; }
    const std::string& IssueSubQualifierAssetBurnAddress() const { return strIssueSubQualifierAssetBurnAddress; }
    const std::string& IssueRestrictedAssetBurnAddress() const { return strIssueRestrictedAssetBurnAddress; }
    const std::string& AddNullQualifierTagBurnAddress() const { return strAddNullQualifierTagBurnAddress; }

    // Addresses
    const std::string& GlobalBurnAddress() const { return strGlobalBurnAddress; }
    const std::string& MiningAddress() const { return strMiningAddress; }
    const std::string& NodeOperatorsAddress() const { return strNodeOperatorsAddress; }
//    const std::string& AmbassadorAddress() const { return strAmbassadorAddress; }
    const std::string& StakingPoolAddress() const { return strStakingPoolAddress; }
    const std::string& ExchangeLiquidityAddress() const { return strExchangeLiquidityAddress; }
    const std::string& BackersAddress() const { return strBackersAddress; }
    const std::string& CompensationAddress() const { return strCompensationAddress; }
    const std::string& CommunityAddress() const { return strCommunityAddress; }
    const std::string& EcosystemGrowthAddress() const { return strEcosystemGrowthAddress; }            
    const std::string& DevTeamAddress() const { return strDevTeamAddress; }
    const std::string& SnapshotAddress() const { return strSnapshotAddress; }
    const std::string& MarketingAddress() const { return strMarketingAddress; }
    const std::string& FoundationReserveAddress() const { return strFoundationReserveAddress; } 
    const std::string& ContributorsAddress() const { return strContributorsAddress; }
    
    /**  Indicates whether or not the provided address is a burn address, some devs keep insert dev address in the IsBurnAddress() function which is potentially dangerous. */
    bool IsBurnAddress(const std::string & p_address) const
    {
        if (
            p_address == strIssueAssetBurnAddress
            || p_address == strReissueAssetBurnAddress
            || p_address == strIssueSubAssetBurnAddress
            || p_address == strIssueUniqueAssetBurnAddress
            || p_address == strIssueMsgChannelAssetBurnAddress
            || p_address == strIssueQualifierAssetBurnAddress
            || p_address == strIssueSubQualifierAssetBurnAddress
            || p_address == strIssueRestrictedAssetBurnAddress
            || p_address == strAddNullQualifierTagBurnAddress
            || p_address == strGlobalBurnAddress
        ) {
            return true;
        }

        return false;
    }

    unsigned int DGWActivationBlock() const { return nDGWActivationBlock; }

    int MaxReorganizationDepth() const { return nMaxReorganizationDepth; }
    int MinReorganizationPeers() const { return nMinReorganizationPeers; }
    int MinReorganizationAge() const { return nMinReorganizationAge; }
    int GetAssetActivationHeight() const { return nAssetActivationHeight; }
    /** SOTER End **/

protected:
    CChainParams() {}

    Consensus::ConsensusParams consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    int nDefaultPort;
    uint64_t nPruneAfterHeight;
    std::vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    int nExtCoinType;
    std::string strNetworkID;
    CBlock genesis;
    std::vector<SeedSpec6> vFixedSeeds;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool fMineBlocksOnDemand;
    bool fMiningRequiresPeers;
    CCheckpointData checkpointData;
    ChainTxData chainTxData;

    /** SOTER Start **/

    // Burn Amounts
    CAmount nIssueAssetBurnAmount;
    CAmount nReissueAssetBurnAmount;
    CAmount nIssueSubAssetBurnAmount;
    CAmount nIssueUniqueAssetBurnAmount;
    CAmount nIssueMsgChannelAssetBurnAmount;
    CAmount nIssueQualifierAssetBurnAmount;
    CAmount nIssueSubQualifierAssetBurnAmount;
    CAmount nIssueRestrictedAssetBurnAmount;
    CAmount nAddNullQualifierTagBurnAmount;

    // Tokenomics Funds
    CAmount nMiningFund;
    CAmount nNodeOperatorsFund;
//    CAmount nAmbassadorFund;
    CAmount nStakingPool;
    CAmount nExchangeLiquidityFund;
    CAmount nBackersFund;
    CAmount nCompensationFund;
    CAmount nCommunityFund;
    CAmount nEcosystemGrowthFund;
    CAmount nDevTeamFund;    
    CAmount nSnapshotFund;
    CAmount nMarketingFund;
    CAmount nFoundationReserveFund;    
    CAmount nContributorsFund;
    CAmount nGlobalBurnFund;
    
    // Burn Addresses
    std::string strIssueAssetBurnAddress;
    std::string strReissueAssetBurnAddress;
    std::string strIssueSubAssetBurnAddress;
    std::string strIssueUniqueAssetBurnAddress;
    std::string strIssueMsgChannelAssetBurnAddress;
    std::string strIssueQualifierAssetBurnAddress;
    std::string strIssueSubQualifierAssetBurnAddress;
    std::string strIssueRestrictedAssetBurnAddress;
    std::string strAddNullQualifierTagBurnAddress;

    // Global Burn Address
    std::string strGlobalBurnAddress;
	
	// Tokenomics Addresses  
    std::string strMiningAddress;
    std::string strNodeOperatorsAddress;
//    std::string strAmbassadorAddress;
    std::string strStakingPoolAddress;
    std::string strExchangeLiquidityAddress;
    std::string strBackersAddress;
    std::string strCompensationAddress;
    std::string strCommunityAddress;
    std::string strEcosystemGrowthAddress;
    std::string strDevTeamAddress;
    std::string strSnapshotAddress;
    std::string strMarketingAddress;
    std::string strFoundationReserveAddress;
    std::string strContributorsAddress;
    
    unsigned int nDGWActivationBlock;
    uint32_t nX12RV2ActivationTime;

    int nMaxReorganizationDepth;
    int nMinReorganizationPeers;
    int nMinReorganizationAge;
    int nAssetActivationHeight;
    /** SOTER End **/
};

/**
 * Creates and returns a std::unique_ptr<CChainParams> of the chosen chain.
 * @returns a CChainParams* of the chosen chain.
 * @throws a std::runtime_error if the chain is not supported.
 */
std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams &Params();

/**
 * Sets the params returned by Params() to those for the given BIP70 chain name.
 * @throws std::runtime_error when the chain is not supported.
 */
void SelectParams(const std::string& chain);

/**
 * Allows modifying the Version Bits regtest parameters.
 */
void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout);

void TurnOffSegwit();

void TurnOffBIP34();

void TurnOffBIP65();

void TurnOffBIP66();

void TurnOffCSV();

#endif // SOTERIA_CHAINPARAMS_H
