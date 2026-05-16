// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-present The Soteria Core developers

#include <iostream>
#include "chainparams.h"
#include "consensus/merkle.h"
#include "consensus/params.h"
#include "tinyformat.h"
#include <util/system.h>
#include "util/strencodings.h"
#include "arith_uint256.h"
#include "base58.h"
#include <assert.h>
#include "chainparamsseeds.h"
#include <limits>
#include <cstdint>      // For uint32_t/uint64_t
#include <array>
#include <string_view>
#include <vector>
#include <utility>
#include <string>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << CScriptNum(0) << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{                           
    const char* pszTimestamp = "E pluribus unum";
    const CScript genesisOutputScript = CScript() << ParseHex("049bc89e0fbeb3f786a5d0b3c508da76377f005338363ee67f7d479f4f3b78e76dd0bf13e25429b991274fac72ecb84a4e0f84aeb6480019e5c56a5101da7df656") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

void CChainParams::TurnOffSegwit()
{
    consensus.nSegwitEnabled = false;
}

void CChainParams::TurnOffCSV()
{
    consensus.nCSVEnabled = false;
}

void CChainParams::TurnOffBIP34()
{
    consensus.nBIP34Enabled = false;
}

void CChainParams::TurnOffBIP65()
{
    consensus.nBIP65Enabled = false;
}

void CChainParams::TurnOffBIP66()
{
    consensus.nBIP66Enabled = false;
}

bool CChainParams::BIP34()
{
    return consensus.nBIP34Enabled;
}

bool CChainParams::BIP65()
{
    return consensus.nBIP34Enabled;
}

bool CChainParams::BIP66()
{
    return consensus.nBIP34Enabled;
}

bool CChainParams::CSVEnabled() const
{
    return consensus.nCSVEnabled;
}

/**
 * Main network
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 0;
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.powLimit = uint256S("000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // 4492800/446568=10 this is more accurate, so will use 11s hardcoded as POW avg in the next release, there is no increase in orphan rates with it.
		consensus.nPowTargetSpacing = 12; 
        consensus.nPowTargetTimespan = 2160; // 180 blocks
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 170;
        consensus.nMinerConfirmationWindow = 180;

        // BIP9 deployments
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1760971167;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1761230367;

        // Soteria network upgrades (hardforks)
        constexpr uint32_t MAX_TIMESTAMP = 2147483647;
        consensus.vUpgrades[Consensus::SOTERG_SWITCH].nTimestamp = 1759415968;
        consensus.vUpgrades[Consensus::SOTERC_SWITCH].nTimestamp = 2147483647; // deactivate
        consensus.vUpgrades[Consensus::SOTERIA_ASSETS].nTimestamp = MAX_TIMESTAMP;
        consensus.vUpgrades[Consensus::SOTERIA_SMART_CONTRACTS].nTimestamp = MAX_TIMESTAMP;
        consensus.vUpgrades[Consensus::SOTERIA_NAME_SYSTEM].nTimestamp = MAX_TIMESTAMP; 
        genesis.nTime = 1759415967;
		consensus.lwmaTimestamp = 1759415968;
        consensus.lwma1Timestamp = 2147483647; // deactivate
        int64_t delta = consensus.lwmaTimestamp - genesis.nTime;
        int height = delta > 0
                ? static_cast<int>(delta / consensus.nPowTargetSpacing)
                : 0;
       
        consensus.lwmaHWCA = height + 5;
        consensus.nBlockTimeDivisor = 6; // min allowed BT
        consensus.nOutboundCycleSeconds = 24 * 60 * 60;
        consensus.nEndCycleMarginPct = 25;
        consensus.nNearBoundaryExtraBlocks = 1;
        consensus.nBurstWindowSeconds = 60;
        consensus.nBurstFactorTenths = 1;
        consensus.lwmaAveragingWindow = 60;
        consensus.lwmaHeight = 1;

		consensus.diffRetargetStartHeight3  = 1; 
		consensus.diffRetargetEndHeight3    = 175000;
		consensus.diffRetargetStartHeight4  = 175000; 
		consensus.diffRetargetEndHeight4    = 1400000;
		consensus.diffRetargetStartHeight = 40000000;
		consensus.diffRetargetEndHeight = 42000000;
		consensus.diffRetargetStartHeight1 = 45000000; 
        consensus.diffRetargetEndHeight1   = 47000000;
		consensus.diffRetargetStartHeight2 = 47000000; 
		consensus.diffRetargetEndHeight2   = 49000000;
		consensus.diffRetargetStartHeight5  = 1400000;
		consensus.diffRetargetEndHeight5    = 40000000;
		
		// decrease the values use f when the block reward will reach 0, to support mining with any CPU so the transactions will keep going at a rate of 9-14s
        consensus.powTypeLimits.emplace_back(uint256S("00000005ffffffffffffffffffffffffffffffffffffffffffffffffffffffff")); // v1.1.2
        consensus.powTypeLimits.emplace_back(uint256S("00000005ffffffffffffffffffffffffffffffffffffffffffffffffffffffff")); 
        // 4 for N, 3 or 2 for L - TEST
        consensus.BIP34LockedIn = 1;

        consensus.nMinimumChainWork = uint256S("00000000000000000000000000000000000000000000000000195d05a106d762"); // TODO UPDATE
        consensus.defaultAssumeValid = uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8"); 

        pchMessageStart[0] = 0x53; 
        pchMessageStart[1] = 0x4F; 
        pchMessageStart[2] = 0x54; 
        pchMessageStart[3] = 0x52; 
        static constexpr int nDefaultPort = 8323; // P2P port
        nPruneAfterHeight = 100000;
        
       genesis = CreateGenesisBlock(1759415967, 31907241, 0x1e00ffff, 4, 18 * COIN / 100);

        consensus.hashGenesisBlock = genesis.GetSOTERGHash();
        assert(consensus.hashGenesisBlock == uint256S("0000001a6714182e55df603ab0232ce6c4b1bef6ef312e5fe40787f02c1477d5"));
        assert(genesis.hashMerkleRoot == uint256S("1ecd95dfb20581f98c3b1a867566fb6318af76de5607f56ae853cccfb01c06f5"));

        // Main seeders
        vSeeds.emplace_back("seed.soteria-network.online", false);
        vSeeds.emplace_back("soterianode.vpnopg.ru", false);
        vSeeds.emplace_back("soteria-demon.favoritcoin.ru", false);
        vSeeds.emplace_back("soter.hashborn.space", false);

        // DNS seeders hardcoded will be added in the next release. TODO
//      vSeeds.emplace_back("dnsseed.us.soteria-network.online", true);
//      vSeeds.emplace_back("dnsseed.eu.soteria-network.online", true);
//      vSeeds.emplace_back("dnsseed.ap.soteria-network.online", true);
		
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,63);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,125);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1,160);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        static constexpr int nExtCoinType = 3000;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        checkpointData = (CCheckpointData) {
            {
                    { 0, uint256S("0000001a6714182e55df603ab0232ce6c4b1bef6ef312e5fe40787f02c1477d5")},
                 { 1500, uint256S("000000014eee83c194a981a06029b29edf6819a6ea9ef183915a9620f886bddc")},
                 { 3000, uint256S("00000001466d04b3d4fa2b695c4c832351a11843f4403074c388b8a6ed3f4855")},
                { 10000, uint256S("0000000003edccc969ff837ace49d6c96a7db108577b83c781ac1800c61f0d74")},
                { 50000, uint256S("0000000045fd082411d0215a9622aa1b61eeb0d8ffdb15bebd6bec39fc1d3ac0")},
               { 100000, uint256S("0000000084100e675aaead560c121003b9bb80fab19449aecb2ad8df03cdfc9b")},
               { 200000, uint256S("00000000242dbf57cc2e41dfd0f7b1e1f45a07718a9e4b1ac8a9aca2c1cb5e47")},
               { 300000, uint256S("000000003417c28de0a82ffeb67ef9678db0dc550f121b1d1b66c513349fb64c")},
               { 400000, uint256S("0000000003af57b6072c3b16e8395233dd112e77ec896acf9f30d77558ad8333")},
               { 500000, uint256S("00000000663bb2d3bf97a629f7201eb8955b2c7b0677efdc75f01846fa24ae4c")},
               { 600000, uint256S("000000000251290289a5327479b88ef1de7ce683911c4c736849786a09e1a7b7")},
               { 700000, uint256S("000000009b078b9388331d295ddebd10f13c3c3f09cb018a2147ccccb41acf85")},
               { 800000, uint256S("0000000108651e8d03279a7951e5e517bc97d2291e59458435ac2bd5b927525e")},
               { 900000, uint256S("0000000210c67c684e425633bf9bbf6a1a94c675156680230cc9ab807e71697d")},
              { 1000000, uint256S("00000000b7c5b7cfaa0bff5f2d5711cf3431f1e9658b43bda3be727d6e938f29")},
              { 1050000, uint256S("00000000ba4f0c8cace65d8bde602c19ee67a6f2e83aa7de3c716de8db47b8ac")},
              { 1100000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")}
//            { 1150000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")}, // To add v1.1.2
//            { 1200000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")},
//            { 1250000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")},
//            { 1300000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")},
//            { 1350000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")},
//            { 1400000, uint256S("000000003dfb9ac3086ddac45db06bb908d62e1251605f8e748bef3735c2ede8")}			
                
            }
        };

        chainTxData = ChainTxData{
            1777648875, 
            1131085,    
            0.08139744759179966           
        };

      // Amounts & Addresses of the Tokenomics
        nMiningFund = 30; // POW Security
	    strMiningAddress = "placeholder";
	    nNodeOperatorsFund = 3;
	    strNodeOperatorsAddress = "placeholder";
	    nStakingPool = 0; // Staking is dynamic depends on supply & demand. Algorithmically adjusted according to the price! // Staking will start at height 2108160 4%.
	    strStakingPoolAddress = "placeholder";
	    nExchangeLiquidityFund = 10;
	    strExchangeLiquidityAddress = "placeholder";
	    nBackersFund = 5;
	    strBackersAddress = "placeholder";
	    nCompensationFund = 2;
	    strCompensationAddress = "placeholder";
	    nCommunityFund = 2;
	    strCommunityAddress = "placeholder";
	    nEcosystemGrowthFund = 15; // Combined R&D/Incubation/Investment Rounds
	    strEcosystemGrowthAddress = "placeholder";
	    nDevTeamFund = 7;
	    strDevTeamAddress = "placeholder";
	    nSnapshotFund = 2;
	    strSnapshotAddress = "placeholder";
	    nMarketingFund = 10;
	    strMarketingAddress = "placeholder";
        nFoundationReserveFund = 70; // 10% actual FoundationReserve, Phase I 10/52, Phase II 10,70.
        strFoundationReserveAddress = "SMy5NT6Qzfwsb6chSks284xugJfcWGhQU7";
        nContributorsFund = 2;
        strContributorsAddress = "placeholder";        
	    nGlobalBurnFund = 0; // This percentage isn't true because we would burn from the profits min 5% max 20%, 2% at 2M height
	    strGlobalBurnAddress = "valid burn address";

        /** SOTER Start **/
        // Burn Amounts
        nIssueAssetBurnAmount = 25 * COIN / 10;
        nReissueAssetBurnAmount = 5 * COIN / 10;
        nIssueSubAssetBurnAmount = 5 * COIN / 10;
        nIssueUniqueAssetBurnAmount = 25 * COIN / 1000;
        nIssueMsgChannelAssetBurnAmount = 5 * COIN / 10;
        nIssueQualifierAssetBurnAmount = 50 * COIN / 10;
        nIssueSubQualifierAssetBurnAmount = 5 * COIN / 10;
        nIssueRestrictedAssetBurnAmount = 75 * COIN / 10;
        nAddNullQualifierTagBurnAmount = 5 * COIN / 10000;

        // Burn Addresses
        strIssueAssetBurnAddress = "RXissueAssetXXXXXXXXXXXXXXXXXhhZGt";
        strReissueAssetBurnAddress = "RXReissueAssetXXXXXXXXXXXXXXVEFAWu";
        strIssueSubAssetBurnAddress = "RXissueSubAssetXXXXXXXXXXXXXWcwhwL";
        strIssueUniqueAssetBurnAddress = "RXissueUniqueAssetXXXXXXXXXXWEAe58";
        strIssueMsgChannelAssetBurnAddress = "RXissueMsgChanneLAssetXXXXXXSjHvAY";
        strIssueQualifierAssetBurnAddress = "RXissueQuaLifierXXXXXXXXXXXXUgEDbC";
        strIssueSubQualifierAssetBurnAddress = "RXissueSubQuaLifierXXXXXXXXXVTzvv5";
        strIssueRestrictedAssetBurnAddress = "RXissueRestrictedXXXXXXXXXXXXzJZ1q";
        strAddNullQualifierTagBurnAddress = "RXaddTagBurnXXXXXXXXXXXXXXXXZQm5ya";

        // DGW Activation
        nDGWActivationBlock = 0;
		
        // Mainnet chain Reorg Settings
        nMaxReorganizationDepth = 120;
        nMinReorganizationPeers = 8;
        nMinReorganizationAge = 15000; // s
        
        nAssetActivationHeight = 1; // Asset activated block height
        /** SOTER End **/
    }
};

/**
 * Testnet (v6)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 0;
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true; 
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.powLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); 
        consensus.nPowTargetTimespan = 2160; // 180 blocks 
        consensus.nPowTargetSpacing = 12;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 135;
        consensus.nMinerConfirmationWindow = 180;

        // BIP9 deployments
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 4294967295;

        // Soteria network upgrades (hardforks)
        constexpr uint32_t MAX_TIMESTAMP = 2147483647;
        consensus.vUpgrades[Consensus::SOTERG_SWITCH].nTimestamp = 1759419050;
        consensus.vUpgrades[Consensus::SOTERC_SWITCH].nTimestamp = 2147483647; // deactivate
        consensus.vUpgrades[Consensus::SOTERIA_ASSETS].nTimestamp = MAX_TIMESTAMP;
        consensus.vUpgrades[Consensus::SOTERIA_SMART_CONTRACTS].nTimestamp = MAX_TIMESTAMP;
        consensus.vUpgrades[Consensus::SOTERIA_NAME_SYSTEM].nTimestamp = MAX_TIMESTAMP; 
  
        // Dual-algo consensus
		consensus.lwmaHeight = 1;
        consensus.lwmaTimestamp = 1759419050;
        consensus.lwmaAveragingWindow = 60;
        consensus.powTypeLimits.emplace_back(uint256S("00000005ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
        consensus.powTypeLimits.emplace_back(uint256S("00000005ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
        consensus.lwma1Timestamp = 2147483647;
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.defaultAssumeValid = uint256S("000000c1936b6133451bb7d064833da83a015337d7b6598d156a451085009cb5");

        pchMessageStart[0] = 0x54; 
        pchMessageStart[1] = 0x6f;
        pchMessageStart[2] = 0x74; 
        pchMessageStart[3] = 0x72; 
        static constexpr int nDefaultPort = 18323;
        static constexpr uint64_t nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1759419049, 12390692, 0x1e00ffff, 4, 18 * COIN / 100);
        consensus.hashGenesisBlock = genesis.GetSOTERGHash();
        assert(consensus.hashGenesisBlock == uint256S("000000c1936b6133451bb7d064833da83a015337d7b6598d156a451085009cb5"));
        assert(genesis.hashMerkleRoot == uint256S("1ecd95dfb20581f98c3b1a867566fb6318af76de5607f56ae853cccfb01c06f5"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,66);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,77);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,79);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Soteria BIP44 cointype in testnet
        static constexpr int nExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        checkpointData = (CCheckpointData) {
            {
                 { 0, uint256S("000000c1936b6133451bb7d064833da83a015337d7b6598d156a451085009cb5")}
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        /** SOTER Start **/
        // Burn Amounts
        nIssueAssetBurnAmount = 25 * COIN / 10;
        nReissueAssetBurnAmount = 5 * COIN / 10;
        nIssueSubAssetBurnAmount = 5 * COIN / 10;
        nIssueUniqueAssetBurnAmount = 25 * COIN / 1000;
        nIssueMsgChannelAssetBurnAmount = 5 * COIN / 10;
        nIssueQualifierAssetBurnAmount = 50 * COIN / 10;
        nIssueSubQualifierAssetBurnAmount = 5 * COIN / 10;
        nIssueRestrictedAssetBurnAmount = 75 * COIN / 10;
        nAddNullQualifierTagBurnAmount = 5 * COIN / 10000;

        nFoundationReserveFund = 10; // Phase I 10/52, Phase II 10,70.
        strFoundationReserveAddress = "n1ZkNVzfrPp4ExfEF5aJbwJbhcBS57dQ3h";

        // Burn Addresses
        strIssueAssetBurnAddress = "n1issueAssetXXXXXXXXXXXXXXXXWdnemQ";
        strReissueAssetBurnAddress = "n1ReissueAssetXXXXXXXXXXXXXXWG9NLd";
        strIssueSubAssetBurnAddress = "n1issueSubAssetXXXXXXXXXXXXXbNiH6v";
        strIssueUniqueAssetBurnAddress = "n1issueUniqueAssetXXXXXXXXXXS4695i";
        strIssueMsgChannelAssetBurnAddress = "n1issueMsgChanneLAssetXXXXXXT2PBdD";
        strIssueQualifierAssetBurnAddress = "n1issueQuaLifierXXXXXXXXXXXXUysLTj";
        strIssueSubQualifierAssetBurnAddress = "n1issueSubQuaLifierXXXXXXXXXYffPLh";
        strIssueRestrictedAssetBurnAddress = "n1issueRestrictedXXXXXXXXXXXXZVT9V";
        strAddNullQualifierTagBurnAddress = "n1addTagBurnXXXXXXXXXXXXXXXXX5oLMH";

        // Global Burn Address
        strGlobalBurnAddress = "n1BurnXXXXXXXXXXXXXXXXXXXXXXU1qejP";

        // DGW Activation
        nDGWActivationBlock = 0;
		
        // Testnet chain Reorg Settings
        nMaxReorganizationDepth = 60;
        nMinReorganizationPeers = 4;
        nMinReorganizationAge = 60 * 60 * 3;

        nAssetActivationHeight = 1; // Asset activated block height
        /** SOTER End **/
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true; 
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.nSubsidyHalvingInterval = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 1080; // nMinerConfirmationWindow * nPowTargetSpacing = 90 * 12 = 1080
        consensus.nPowTargetSpacing = 12;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 65; 
        consensus.nMinerConfirmationWindow = 90; 

        // BIP9 deployments
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;

        // Soteria network
        constexpr uint32_t MAX_TIMESTAMP = 2147483647;
        consensus.vUpgrades[Consensus::SOTERG_SWITCH].nTimestamp = 1759421432; 
        consensus.vUpgrades[Consensus::SOTERC_SWITCH].nTimestamp = 2147483647; // deactivate
        consensus.vUpgrades[Consensus::SOTERIA_ASSETS].nTimestamp = MAX_TIMESTAMP; 
        consensus.vUpgrades[Consensus::SOTERIA_SMART_CONTRACTS].nTimestamp = MAX_TIMESTAMP; 
        consensus.vUpgrades[Consensus::SOTERIA_NAME_SYSTEM].nTimestamp = MAX_TIMESTAMP; 

        consensus.lwmaTimestamp = 1759421432;        
        consensus.lwmaAveragingWindow = 180; 
        consensus.powTypeLimits.emplace_back(uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));  
        consensus.powTypeLimits.emplace_back(uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));   

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("606c795ca9d9ee08dba32d599dd65af25ba9e0b9aaeabc4b8a43533805e43136");

       pchMessageStart[0] = 0x72; 
       pchMessageStart[1] = 0x74; 
       pchMessageStart[2] = 0x6f; 
       pchMessageStart[3] = 0x73; 
       static constexpr int nDefaultPort = 18310;
       static constexpr uint64_t nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1759421431, 1, 0x207fffff, 4, 18 * COIN / 100);
        consensus.hashGenesisBlock = genesis.GetSOTERGHash();
        assert(consensus.hashGenesisBlock == uint256S("606c795ca9d9ee08dba32d599dd65af25ba9e0b9aaeabc4b8a43533805e43136"));
        assert(genesis.hashMerkleRoot == uint256S("1ecd95dfb20581f98c3b1a867566fb6318af76de5607f56ae853cccfb01c06f5"));

        vFixedSeeds.clear(); 
        vSeeds.clear();      

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fMiningRequiresPeers = false;

        checkpointData = (CCheckpointData) {
            {
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,60);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,122);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Soteria BIP44 cointype in testnet
        static constexpr int nExtCoinType = 1;

        /** SOTER Start **/
        // Burn Amounts
        nIssueAssetBurnAmount = 25 * COIN / 10;
        nReissueAssetBurnAmount = 5 * COIN / 10;
        nIssueSubAssetBurnAmount = 5 * COIN / 10;
        nIssueUniqueAssetBurnAmount = 25 * COIN / 10;
        nIssueMsgChannelAssetBurnAmount = 5 * COIN / 10;
        nIssueQualifierAssetBurnAmount = 50 * COIN / 10;
        nIssueSubQualifierAssetBurnAmount = 5 * COIN / 10;
        nIssueRestrictedAssetBurnAmount = 75 * COIN / 10;
        nAddNullQualifierTagBurnAmount = 5 * COIN / 10000;

        // Burn Addresses
        strIssueAssetBurnAddress = "n1issueAssetXXXXXXXXXXXXXXXXWdnemQ";
        strReissueAssetBurnAddress = "n1ReissueAssetXXXXXXXXXXXXXXWG9NLd";
        strIssueSubAssetBurnAddress = "n1issueSubAssetXXXXXXXXXXXXXbNiH6v";
        strIssueUniqueAssetBurnAddress = "n1issueUniqueAssetXXXXXXXXXXS4695i";
        strIssueMsgChannelAssetBurnAddress = "n1issueMsgChanneLAssetXXXXXXT2PBdD";
        strIssueQualifierAssetBurnAddress = "n1issueQuaLifierXXXXXXXXXXXXUysLTj";
        strIssueSubQualifierAssetBurnAddress = "n1issueSubQuaLifierXXXXXXXXXYffPLh";
        strIssueRestrictedAssetBurnAddress = "n1issueRestrictedXXXXXXXXXXXXZVT9V";
        strAddNullQualifierTagBurnAddress = "n1addTagBurnXXXXXXXXXXXXXXXXX5oLMH";

        // Global Burn Address
        strGlobalBurnAddress = "n1BurnXXXXXXXXXXXXXXXXXXXXXXU1qejP";

        // DGW Activation
        nDGWActivationBlock = 0;
		
        // Regtest chain Reorg Settings
        nMaxReorganizationDepth = 40;
        nMinReorganizationPeers = 2;
        nMinReorganizationAge = 60 * 60 * 6;

        nAssetActivationHeight = 1; // Asset activated block height
        /** SOTER End **/
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams& Params()
{
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}

void TurnOffSegwit()
{
    globalChainParams->TurnOffSegwit();
}

void TurnOffCSV()
{
    globalChainParams->TurnOffCSV();
}

void TurnOffBIP34()
{
    globalChainParams->TurnOffBIP34();
}

void TurnOffBIP65()
{
    globalChainParams->TurnOffBIP65();
}

void TurnOffBIP66()
{
    globalChainParams->TurnOffBIP66();
}
