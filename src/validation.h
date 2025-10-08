// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTER_VALIDATION_H
#define SOTER_VALIDATION_H

#if defined(HAVE_CONFIG_H)
#include "config/soteria-config.h"
#endif

#include "amount.h"
#include "chain.h"
#include "coins.h"
#include "fs.h"
#include "policy/policy.h" // For RECOMMENDED_MIN_TX_FEE
#include "protocol.h" // For CMessageHeader::MessageStartChars
#include "policy/feerate.h"
#include "script/script_error.h"
#include "sync.h"
#include "versionbits.h"
#include "spentindex.h"
#include "addressindex.h"
#include "timestampindex.h"
#include <txdb.h>
#include <txmempool.h>  // For CTxMemPool::cs
#include <algorithm>
#include <atomic>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <stdint.h>
#include <list>
#include <string>
#include <utility>
#include <vector>
#include <assets/assets.h>
#include <assets/assetdb.h>
#include <assets/messages.h>
#include <assets/myassetsdb.h>
#include <assets/restricteddb.h>
#include <assets/assetsnapshotdb.h>
#include <assets/snapshotrequestdb.h>

class CBlockIndex;
class CBlockTreeDB;
class CChainParams;
class CCoinsViewDB;
class CInv;
class CConnman;
class CScriptCheck;
class CBlockPolicyEstimator;
class CTxMemPool;
class CValidationState;
class CTxUndo;
struct ChainTxData;

class CAssetsDB;
class CAssets;
class CSnapshotRequestDB;

struct PrecomputedTransactionData;
struct LockPoints;

/** Default for -whitelistrelay. */
static constexpr bool DEFAULT_WHITELISTRELAY = true;
/** Default for -whitelistforcerelay. */
static constexpr bool DEFAULT_WHITELISTFORCERELAY = true;
/** Default for -minrelaytxfee, minimum relay fee for transactions*/
static constexpr unsigned int DEFAULT_MIN_RELAY_TX_FEE = 1000000; // COIN / 10, 0.1 // def=1000000, 0.01
//! -maxtxfee default (hard cap per TX)
static constexpr CAmount DEFAULT_TRANSACTION_MAXFEE = 1000 * COIN; // def=1000
//! Discourage users to set fees higher than this amount (in soterios) per kB (soft-warn)
static constexpr CAmount HIGH_TX_FEE_PER_KB = 0.1 * COIN; //  10 * COIN, def= 0.1 * COIN
//! -maxtxfee will warn if called with a higher fee than this amount (in soterios)
static constexpr CAmount HIGH_MAX_TX_FEE = 100 * HIGH_TX_FEE_PER_KB;
/** Default for -limitancestorcount, max number of in-mempool ancestors */
static constexpr unsigned int DEFAULT_ANCESTOR_LIMIT = 200;
/** Default for -limitancestorsize, maximum kilobytes of tx + all in-mempool ancestors */
static constexpr unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT = 250;
/** Default for -limitdescendantcount, max number of in-mempool descendants */
static constexpr unsigned int DEFAULT_DESCENDANT_LIMIT = 200;
/** Default for -limitdescendantsize, maximum kilobytes of in-mempool descendants */
static constexpr unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT = 250;
/** Default for -cache.memoryPoolExpiry, nothing above one week, expiration time for mempool transactions in hours */
static constexpr unsigned int DEFAULT_MEMPOOL_EXPIRY = 72; // default in BTC=336, oD,nN=72. 24,48. 15*2016=8.4, nDin=24
/** Maximum kilobytes for transactions to store for processing during reorg.
• Stores up to 20 000 “orphan” transactions awaiting parents. • With 4 MB blocks we might see bursts of orphans if many in‐flight parents arrive late—consider increasing to 50 000 if we notice frequent drop-outs. */
static constexpr unsigned int MAX_DISCONNECTED_TX_POOL_SIZE = 60000; // def=20000
/** The maximum size of a blk?????.dat file (since 0.8) */ 
/** Scaling for 3 MB Blocks: Reduces file‐rotation overhead by half. Aligns chunk sizes to contain several blocks. Keeps power-of-two boundaries for filesystem efficiency */
static constexpr unsigned int MAX_BLOCKFILE_SIZE = 0x10000000; // def=128 MiB, 256 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static constexpr unsigned int BLOCKFILE_CHUNK_SIZE = 0x2000000; // def=16 MiB, 32 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static constexpr unsigned int UNDOFILE_CHUNK_SIZE = 0x400000; // def=1 MiB, 4 MiB
/** Skip PoW testing headers until this blockheight */
// static const int SKIP_BLOCKHEADER = 100000; // Skip up to last updated checkpoint, activate in the future

/** Maximum number of script-checking threads allowed */
static constexpr int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static constexpr int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. ~32×0.32s ≃ 10s */
static constexpr int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 256; // 16nR, 32oD, 128nD, 256nV. 32 in-smart = 32 × 4MB = 128MB RAM
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
/** A 2 second timeout is only 13 percent of a 15 second block interval. At 4 seconds it becomes 27 percent, giving peers more breathing room to finish sending large blocks before we drop them */
static constexpr unsigned int BLOCK_STALLING_TIMEOUT = 4; /** 4R increase with no.peers to avoid false positives and give a litle more headroom than def=2 for slow peers */
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  less than this number, we reached its tip. Changing this value is a protocol upgrade. messages (~ 160 bytes × 2000 ≃ 320 KiB)*/
static constexpr unsigned int MAX_HEADERS_RESULTS = 10000; /** default=2000, 10000 oD 80 KiB under ~1 MiB, 20000 experimental, 20K above ~1.5MiB so we should consider it carefully because it increases DOS */
/** Maximum depth of blocks we're willing to serve as compact blocks to peers
 *  when requested. For older blocks, a regular BLOCK response will be sent. def=5 */
static constexpr int MAX_CMPCTBLOCK_DEPTH = 64;
/** Maximum depth of blocks we're willing to respond to GETBLOCKTXN requests for. def =10*/
static constexpr int MAX_BLOCKTXN_DEPTH = 64;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and pruning harder). We'll probably
 *  want to make this a per-peer adaptive value at some point. */
/**   If your nodes have limited RAM (< 4 GB), lower this to 1024–2048.  If we want faster initial sync on beefy hardware, you can push this higher to 6144-8192. */
static constexpr unsigned int BLOCK_DOWNLOAD_WINDOW = 4096; // ~17 hours coverage
/** Time to wait (in seconds) between writing blocks/block index to disk. */
static constexpr unsigned int DATABASE_WRITE_INTERVAL = 60 * 6;
/** Time to wait (in seconds) between flushing chainstate to disk. */
static constexpr unsigned int DATABASE_FLUSH_INTERVAL = 24 * 60 * 6;
/** Time to wait (in seconds) between flushing to database if in speedy sync interval, default=10 */
static constexpr unsigned int DATABASE_FLUSH_INTERVAL_SPEEDY = 60 * 5;
/** Maximum length of reject messages. */
static constexpr unsigned int MAX_REJECT_MESSAGE_LENGTH = 111;
/** Average delay between local address broadcasts in seconds. */
static constexpr unsigned int AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL = 24 * 60 * 60;
/** Average delay between peer address broadcasts in seconds. */
static constexpr unsigned int AVG_ADDRESS_BROADCAST_INTERVAL = 30;
/** Average delay between trickled inventory transmissions in seconds.
 *  Blocks and whitelisted receivers bypass this, outbound peers get half this delay. */
static constexpr unsigned int INVENTORY_BROADCAST_INTERVAL = 15;
/** Maximum number of inventory items to send per transmission.
 *  Limits the impact of low-fee transaction floods. */
/** Fewer INV announcements per block translates to:
    Peers learn about new transactions more slowly
    Higher membrane effect—your mempool “leaks” updates in longer pauses
    Increased risk of orphaned transactions and blocks as peers lag behind. */ 
static constexpr unsigned int INVENTORY_BROADCAST_MAX = 4 * 7 * INVENTORY_BROADCAST_INTERVAL;
//    = 4     // blockSizeMB
//    * 7     // INV per second per MB
//    * INVENTORY_BROADCAST_INTERVAL;
/** Average delay between feefilter broadcasts in seconds. */
static constexpr unsigned int AVG_FEEFILTER_BROADCAST_INTERVAL = 4 * 60;
/** Maximum feefilter broadcast delay after significant change. */
static constexpr unsigned int MAX_FEEFILTER_CHANGE_DELAY = 2 * 60;
/** Block download timeout base, expressed in millionths of the block interval (i.e. 10 min) */ // TODO Should we change this, with 1 minutes block intervals?
static constexpr int64_t BLOCK_DOWNLOAD_TIMEOUT_BASE = 10000000;  // 150s
/** Additional block download timeout per parallel downloading peer (i.e. 5 min) */
static constexpr int64_t BLOCK_DOWNLOAD_TIMEOUT_PER_PEER = 5000000;  //  75s

// Check if Dual Algo is activated at given point
bool IsDualAlgoEnabled(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& params);

static const int64_t DEFAULT_MAX_TIP_AGE = 60 * 60 * 12; /** 144 blocks = 2160, in nReb is 18, in oReb is 36, at the start 6-24h, 72 blocks in LLC. 2160, for fast chains better to choose 72 than 144 conserv */
// ~144 blocks behind -> 2 x fork detection time, was 24 * 60 * 60in v1, in 2.5*6, in v2=18*60 
/** Maximum age of our tip in seconds for us to be considered current for fee estimation */
static constexpr int64_t MAX_FEE_ESTIMATION_TIP_AGE = 2 * 60 * 60;

/** Default for -permitbaremultisig */
static constexpr bool DEFAULT_PERMIT_BAREMULTISIG = true;
static constexpr bool DEFAULT_CHECKPOINTS_ENABLED = true;
static constexpr bool DEFAULT_TXINDEX = false;
static constexpr bool DEFAULT_ASSETINDEX = false;
static constexpr bool DEFAULT_ADDRESSINDEX = false;
static constexpr bool DEFAULT_TIMESTAMPINDEX = false;
static constexpr bool DEFAULT_SPENTINDEX = false;
static constexpr bool DEFAULT_REWARDS_ENABLED = false;
/** Default for -dbmaxfilesize, in MB, If our node handles a heavier transaction rate (bigger blocks, faster cadence), we might increase this to 4 MB or 8 MB so we write fewer files, but the default 2 MB is a sane starting point. */
static constexpr int64_t DEFAULT_DB_MAX_FILE_SIZE = 2;

static constexpr unsigned int DEFAULT_BANSCORE_THRESHOLD = 100;
/** Default for -persistmempool */
static constexpr bool DEFAULT_PERSIST_MEMPOOL = true;
/** Default for -mempoolreplacement */
static constexpr bool DEFAULT_ENABLE_REPLACEMENT = true;
/** Default for using fee filter */
static constexpr bool DEFAULT_FEEFILTER = true;

/** Maximum number of headers to announce when relaying blocks with headers message.*/
static constexpr unsigned int MAX_BLOCKS_TO_ANNOUNCE = 120;

/** Maximum number of unconnecting headers announcements before DoS score */
/** def=10 but for 15s this can cause false positives and disrupt honest peers, recom 15-20min, security risk is negligible in 15s but the benefit is real, we need to address a pain point for nodes on a fast chain!  */
static constexpr int MAX_UNCONNECTING_HEADERS = 20; 

static constexpr bool DEFAULT_PEERBLOOMFILTERS = true;

static constexpr uint64_t DEFAULT_MAX_REORG_LENGTH = 100;

/** Default for -stopatheight */
static constexpr int DEFAULT_STOPATHEIGHT = 0;

struct BlockHasher
{
    size_t operator()(const uint256& hash) const { return hash.GetCheapHash(); }
};

extern CScript COINBASE_FLAGS;
extern CCriticalSection cs_main;
extern CBlockPolicyEstimator feeEstimator;
extern CTxMemPool mempool;
// extern std::atomic_bool g_is_mempool_loaded;
typedef std::unordered_map<uint256, CBlockIndex*, BlockHasher> BlockMap;
extern BlockMap mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockWeight;
extern const std::string strMessageMagic;
extern CWaitableCriticalSection csBestBlock;
extern CConditionVariable cvBlockChange;
extern std::atomic_bool fImporting;
extern std::atomic_bool fReindex;
extern bool fMessaging;
extern bool fRestricted;
extern int nScriptCheckThreads;
extern bool fTxIndex;
extern bool fAssetIndex;
extern bool fAddressIndex;
extern bool fSpentIndex;
extern bool fTimestampIndex;
extern bool fIsBareMultisigStd;
extern bool fRequireStandard;
extern bool fCheckBlockIndex;
extern bool fCheckpointsEnabled;
extern size_t nCoinCacheUsage;
/** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
extern CFeeRate minRelayTxFee;
/** Absolute maximum transaction fee (in soterios) used by wallet and mempool (rejects high fee in sendrawtransaction) */
extern CAmount maxTxFee;

extern bool fUnitTest;

/** If the tip is older than this (in seconds), the node is considered to be in initial block download. */
extern int64_t nMaxTipAge;
extern bool fEnableReplacement;

/** Block hash whose ancestors we will assume to have valid scripts without checking them. */
extern uint256 hashAssumeValid;

/** Minimum work we will assume exists on some valid chain. */
extern arith_uint256 nMinimumChainWork;

/** Best header we've seen so far (used for getheaders queries' starting points). */
extern CBlockIndex *pindexBestHeader;

/** Minimum disk space required - used in CheckDiskSpace() */
static constexpr uint64_t nMinDiskSpace = 52428800;

/** Pruning-related variables and constants */
/** True if any block files have ever been pruned. */
extern bool fHavePruned;
/** True if we're running in -prune mode. */
extern bool fPruneMode;
/** Number of MiB of block files that we're trying to stay below. */
extern uint64_t nPruneTarget;
/** Block files containing a block-height within MIN_BLOCKS_TO_KEEP of chainActive.Tip() will not be pruned. Default 288 was tuned for Bitcoin’s 10 min blocks (~2 days of history). */
static constexpr unsigned int MIN_BLOCKS_TO_KEEP = 11520;
/** Default 6 covers one 1 hr window on Bitcoin (10 min × 6). On startup, how many blocks from the tip to re-verify for corruption/consensus. For 15 s blocks, 6 blocks is only 90 s—probably too shallow. */
static constexpr signed int DEFAULT_CHECKBLOCKS = 240;
/** The intensity of the disk integrity check (0–4). A qualitative setting (how thoroughly to scan files) and rarely needs changing. */
static constexpr unsigned int DEFAULT_CHECKLEVEL = 3;

// Require that user allocate at least 1417.5MB for block & undo files (blk???.dat and rev???.dat)
// At 4MB per block, 288 blocks = 864MB.
// Add 15% for Undo data = 993MB
// Add 20% for Orphan block rate = 1191MB
// We want the low water mark after pruning to be at least 1191 MB and since we prune in
// full block file chunks, we need the high water mark which triggers the prune to be
// one 128MB block file + added 15% undo data = 147MB greater for a total of 1377MB
// Setting the target to > than 1418MB will make it likely we can respect the target.
static constexpr uint64_t MIN_DISK_SPACE_FOR_BLOCK_FILES = 2200ULL * 1024 * 1024 * 1024;
extern uint64_t nMaxReorgLength;
/** 
 * Process an incoming block. This only returns after the best known valid
 * block is made active. Note that it does not, however, guarantee that the
 * specific block passed to it has been checked for validity!
 *
 * If you want to *possibly* get feedback on whether pblock is valid, you must
 * install a CValidationInterface (see validationinterface.h) - this will have
 * its BlockChecked method called whenever *any* block completes validation.
 *
 * Note that we guarantee that either the proof-of-work is valid on pblock, or
 * (and possibly also) BlockChecked will have been called.
 * 
 * Call without cs_main held.
 *
 * @param[in]   pblock  The block we want to process.
 * @param[in]   fForceProcessing Process this block even if unrequested; used for non-network block sources and whitelisted peers.
 * @param[out]  fNewBlock A boolean which is set to indicate if the block was first received via this call
 * @return True if state.IsValid()
 */
bool ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool* fNewBlock) LOCKS_EXCLUDED(cs_main);

/**
 * Process incoming block headers.
 *
 * Call without cs_main held.
 *
 * @param[in]  block The block headers themselves
 * @param[out] state This may be set to an Error state if any error occurred processing them
 * @param[in]  chainparams The params for the chain we want to connect to
 * @param[out] ppindex If set, the pointer will be set to point to the last new block index object for the given headers
 * @param[out] first_invalid First header that fails validation, if one exists
 */
bool ProcessNewBlockHeaders(const std::vector<CBlockHeader>& block, CValidationState& state, const CChainParams& chainparams, const CBlockIndex** ppindex=nullptr, CBlockHeader *first_invalid = nullptr) LOCKS_EXCLUDED(cs_main);

/** Check whether enough disk space is available for an incoming block */
bool CheckDiskSpace(uint64_t nAdditionalBytes = 0);
/** Open a block file (blk?????.dat) */
FILE* OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);
/** Translation to a filesystem path */
fs::path GetBlockPosFilename(const CDiskBlockPos &pos, const char *prefix);
/** Import blocks from an external file */
bool LoadExternalBlockFile(const CChainParams& chainparams, FILE* fileIn, CDiskBlockPos *dbp = nullptr);
/** Ensures we have a genesis block in the block tree, possibly writing one to disk. */
bool LoadGenesisBlock(const CChainParams& chainparams);
/** Load the block tree and coins database from disk,
 * initializing state if we're running with -reindex. */
bool LoadBlockIndex(const CChainParams& chainparams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
/** Update the chain tip based on database information. */
bool LoadChainTip(const CChainParams& chainparams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
/** Unload database information */
void UnloadBlockIndex();
/** Run an instance of the script checking thread */
void ThreadScriptCheck();
/** Check whether we are doing an initial block download (synchronizing from disk or network) */
bool IsInitialBlockDownload();
bool IsInitialSyncSpeedUp();
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(const uint256 &hash, CTransactionRef &tx, const Consensus::ConsensusParams& params, uint256 &hashBlock, bool fAllowSlow = false);
/** Find the best known block, and make it the tip of the block chain */
bool ActivateBestChain(CValidationState& state, const CChainParams& chainparams, std::shared_ptr<const CBlock> pblock = std::shared_ptr<const CBlock>());
CAmount GetBlockSubsidy(int nHeight, const Consensus::ConsensusParams& consensusParams);

/** Guess verification progress (as a fraction between 0.0=genesis and 1.0=current tip). */
double GuessVerificationProgress(const ChainTxData& data, CBlockIndex* pindex);

/** Calculate the amount of disk space the block & undo files currently use */
uint64_t CalculateCurrentUsage();

/**
 *  Mark one block file as pruned.
 */
void PruneOneBlockFile(const int fileNumber) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 *  Actually unlink the specified files
 */
void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune);

/** Create a new block index entry for a given block hash */
CBlockIndex * InsertBlockIndex(uint256 hash);
/** Flush all state, indexes and buffers to disk. */
void FlushStateToDisk();
/** Prune block files and flush state to disk. */
void PruneAndFlush();
/** Prune block files up to a given height */
void PruneBlockFilesManual(int nManualPruneHeight);

/** Check is UAHF has activated.*/
 bool IsUAHFenabled(const CBlockIndex *pindexPrev);
 bool IsUAHFenabledForCurrentBlock();

/** (try to) add transaction to memory pool
 * plTxnReplaced will be appended to with all transactions replaced from mempool **/
bool AcceptToMemoryPool(CTxMemPool& pool, CValidationState &state, const CTransactionRef &tx,
                        bool* pfMissingInputs, std::list<CTransactionRef>* plTxnReplaced,
                        bool bypass_limits, const CAmount nAbsurdFee, bool test_accept=false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Convert CValidationState to a human-readable message for logging */
std::string FormatStateMessage(const CValidationState &state);

/** Get the BIP9 state for a given deployment at the current tip. */
ThresholdState VersionBitsTipState(const Consensus::ConsensusParams& params, Consensus::DeploymentPos pos);

/** Get the numerical statistics for the BIP9 state for a given deployment at the current tip. */
BIP9Stats VersionBitsTipStatistics(const Consensus::ConsensusParams& params, Consensus::DeploymentPos pos);

/** Get the block height at which the BIP9 deployment switched into the state for the block building on the current tip. */
int VersionBitsTipStateSinceHeight(const Consensus::ConsensusParams& params, Consensus::DeploymentPos pos);


/** Apply the effects of this transaction on the UTXO set represented by view */
void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, int nHeight);

void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo& txundo, int nHeight, uint256 blockHash, CAssetsCache* assetCache = nullptr, std::pair<std::string, CBlockAssetUndo>* undoAssetData = nullptr);

/** Transaction validation functions */

/**
 * Check if transaction will be final in the next block to be created.
 *
 * Calls IsFinalTx() with current block height and appropriate block time.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckFinalTx(const CTransaction &tx, int flags = -1) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Test whether the LockPoints height and time are still valid on the current chain
 */
bool TestLockPointValidity(const LockPoints* lp) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Check if transaction will be BIP 68 final in the next block to be created.
 *
 * Simulates calling SequenceLocks() with data from the tip of the current active chain.
 * Optionally stores in LockPoints the resulting height and time calculated and the hash
 * of the block needed for calculation or skips the calculation and uses the LockPoints
 * passed in for evaluation.
 * The LockPoints should not be considered valid if CheckSequenceLocks returns false.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckSequenceLocks(const CTransaction &tx, int flags, LockPoints* lp = nullptr, bool useExistingLockPoints = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Closure representing one script verification
 * Note that this stores references to the spending transaction
 */
class CScriptCheck
{
private:
    CTxOut m_tx_out;
    const CTransaction *ptxTo;
    unsigned int nIn;
    unsigned int nFlags;
    bool cacheStore;
    ScriptError error;
    PrecomputedTransactionData *txdata;

public:
    CScriptCheck(): ptxTo(nullptr), nIn(0), nFlags(0), cacheStore(false), error(SCRIPT_ERR_UNKNOWN_ERROR) {}
    CScriptCheck(const CTxOut& outIn, const CTransaction& txToIn, unsigned int nInIn, unsigned int nFlagsIn, bool cacheIn, PrecomputedTransactionData* txdataIn) :
        m_tx_out(outIn), ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), cacheStore(cacheIn), error(SCRIPT_ERR_UNKNOWN_ERROR), txdata(txdataIn) { }

    bool operator()();

    void swap(CScriptCheck &check) {
        std::swap(ptxTo, check.ptxTo);
        std::swap(m_tx_out, check.m_tx_out);
        std::swap(nIn, check.nIn);
        std::swap(nFlags, check.nFlags);
        std::swap(cacheStore, check.cacheStore);
        std::swap(error, check.error);
        std::swap(txdata, check.txdata);
    }

    ScriptError GetScriptError() const { return error; }
};

/** Initializes the script-execution cache */
void InitScriptExecutionCache();

bool GetTimestampIndex(const unsigned int &high, const unsigned int &low, const bool fActiveOnly, std::vector<std::pair<uint256, unsigned int> > &hashes);
bool GetSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value);
bool HashOnchainActive(const uint256 &hash);
bool GetAddressIndex(uint160 addressHash, int type, std::string assetName,
                     std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex,
                     int start = 0, int end = 0);
bool GetAddressIndex(uint160 addressHash, int type,
                     std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex,
                     int start = 0, int end = 0);
bool GetAddressUnspent(uint160 addressHash, int type, std::string assetName,
                       std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs);
bool GetAddressUnspent(uint160 addressHash, int type,
                       std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs);

/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const CDiskBlockPos& pos, const Consensus::ConsensusParams& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::ConsensusParams& consensusParams);

/** Functions for validating blocks and updating the block tree */

/** Context-independent validity checks */
bool CheckBlock(const CBlock& block, CValidationState& state, const Consensus::ConsensusParams& consensusParams, bool fCheckPOW = true, bool fCheckMerkleRoot = true, bool fCheckAssetDuplicate = true, bool fForceDuplicateCheck = true);

/** Check a block is completely valid from start to finish (only works on top of our current best block, with cs_main held) */
bool TestBlockValidity(CValidationState& state, const CChainParams& chainparams, const CBlock& block, CBlockIndex* pindexPrev, bool fCheckPOW = true, bool fCheckMerkleRoot = true) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Check whether witness commitments are required for block. */
bool IsWitnessEnabled(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& params);

/** When there are blocks in the active chain with missing data, rewind the chainstate and remove them from the block index */
bool RewindBlockIndex(const CChainParams& params) LOCKS_EXCLUDED(cs_main);

/** Update uncommitted block structures (currently: only the witness nonce). This is safe for submitted blocks. */
void UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& consensusParams);

/** Produce the necessary coinbase commitment for a block (modifies the hash, don't call for mined blocks). */
std::vector<unsigned char> GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& consensusParams);

/** RAII wrapper for VerifyDB: Verify consistency of the block and coin databases */
class CVerifyDB {
public:
    CVerifyDB();
    ~CVerifyDB();
    bool VerifyDB(const CChainParams& chainparams, CCoinsView *coinsview, int nCheckLevel, int nCheckDepth);
};

/** Replay blocks that aren't fully applied to the database. */
bool ReplayBlocks(const CChainParams& params, CCoinsView* view);

/** Find the last common block between the parameter chain and a locator. */
CBlockIndex* FindForkInGlobalIndex(const CChain& chain, const CBlockLocator& locator) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Mark a block as precious and reorganize. */
bool PreciousBlock(CValidationState& state, const CChainParams& params, CBlockIndex *pindex) LOCKS_EXCLUDED(cs_main);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState& state, const CChainParams& chainparams, CBlockIndex *pindex) LOCKS_EXCLUDED(cs_main);

/** Remove invalidity status from a block and its descendants. */
bool ResetBlockFailureFlags(CBlockIndex *pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** The currently-connected chain of blocks (protected by cs_main). */
extern CChain chainActive;

/** Global variable that points to the coins database (protected by cs_main) */
extern CCoinsViewDB *pcoinsdbview;

/** Global variable that points to the active CCoinsView (protected by cs_main) */
extern CCoinsViewCache *pcoinsTip;

/** Global variable that points to the active block tree (protected by cs_main) */
extern CBlockTreeDB *pblocktree;

/** SOTER START */
/** Global variable that point to the active assets database (protected by cs_main) */
extern CAssetsDB *passetsdb;

/** Global variable that point to the active assets (protected by cs_main) */
extern CAssetsCache *passets;

/** Global variable that point to the assets metadata LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, CDatabasedAssetData> *passetsCache;

/** Global variable that points to the subscribed channel LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, CMessage> *pMessagesCache;

/** Global variable that points to the subscribed channel LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, int> *pMessageSubscribedChannelsCache;

/** Global variable that points to the address seen LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, int> *pMessagesSeenAddressCache;

/** Global variable that points to the messages database (protected by cs_main) */
extern CMessageDB *pmessagedb;

/** Global variable that points to the message channel database (protected by cs_main) */
extern CMessageChannelDB *pmessagechanneldb;

/** Global variable that points to my wallets restricted database (protected by cs_main) */
extern CMyRestrictedDB *pmyrestricteddb;

/** Global variable that points to the active restricted asset database (protected by cs_main) */
extern CRestrictedDB *prestricteddb;

/** Global variable that points to the asset verifier LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, CNullAssetTxVerifierString> *passetsVerifierCache;

/** Global variable that points to the asset address qualifier LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, int8_t> *passetsQualifierCache; // hash(address,qualifier_name) ->int8_t

/** Global variable that points to the asset address restriction LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, int8_t> *passetsRestrictionCache; // hash(address,qualifier_name) ->int8_t

/** Global variable that points to the global asset restriction LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, int8_t> *passetsGlobalRestrictionCache;

/** Global variable that point to the active Snapshot Request database (protected by cs_main) */
extern CSnapshotRequestDB *pSnapshotRequestDb;

/** Global variable that point to the active asset snapshot database (protected by cs_main) */
extern CAssetSnapshotDB *pAssetSnapshotDb;

extern CDistributeSnapshotRequestDB *pDistributeSnapshotDb;
/** SOTER END */

/**
 * Return the spend height, which is one more than the inputs.GetBestBlock().
 * While checking, GetBestBlock() refers to the parent block. (protected by cs_main)
 * This is also true for mempool checks.
 */
int GetSpendHeight(const CCoinsViewCache& inputs);

extern VersionBitsCache versionbitscache;

/**
 * Determine what nVersion a new block should use.
 */
int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::ConsensusParams& params);

/** Reject codes greater or equal to this can be returned by AcceptToMemPool
 * for transactions, to signal internal conditions. They cannot and should not
 * be sent over the P2P network.
 */
static const unsigned int REJECT_INTERNAL = 0x100;
/** Too high fee. Can not be triggered by P2P transactions */
static const unsigned int REJECT_HIGHFEE = 0x100;

/** Get block file info entry for one block file */
CBlockFileInfo* GetBlockFileInfo(size_t n);

/** Dump the mempool to disk. */
bool DumpMempool();

/** Load the mempool from disk. */
bool LoadMempool();

/** SOTER START */
bool AreAssetsDeployed();

bool AreSmartContractsDeployed();

bool AreMessagesDeployed();

bool AreRestrictedAssetsDeployed();

bool AreEnforcedValuesDeployed();

bool AreCoinbaseCheckAssetsDeployed();

bool IsSoteriaNameSystemDeployed();

// Only used by test framework
void SetEnforcedValues(bool value);
void SetEnforcedCoinbase(bool value);

bool IsDGWActive(unsigned int nBlockNumber);

CAssetsCache* GetCurrentAssetCache();
/** SOTER END */

#endif // SOTERIA_VALIDATION_H
