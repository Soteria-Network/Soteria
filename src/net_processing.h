// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_NET_PROCESSING_H
#define SOTERIA_NET_PROCESSING_H

#include <net.h>
#include <validationinterface.h>
#include <consensus/params.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
/** Default for -maxorphantx, maximum number of orphan transactions kept in memory. 
 increasing it cause fewer valid transactions to drop when parents arrive late, each orphan carries ~400B, so 1000 orphans use ~400KiB. Very safe for most nodes*/
static constexpr unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS = 4000; // 5000, 6000, 8000, 10000 in the future
/** Expiration time for orphan transactions in seconds */
static constexpr int64_t ORPHAN_TX_EXPIRE_TIME = 480; // 420, 360, 300 in the future
/** Minimum time between orphan transactions expire time checks in seconds */
static constexpr int64_t ORPHAN_TX_EXPIRE_INTERVAL = 240; // 180 in the future
/** Default number of orphan+recently-replaced txn to keep around for block reconstruction */
static constexpr unsigned int DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN = 4000; // 5000, 6000, 8000, 10000 in the future
/** Headers download timeout expressed in microseconds
 *  Timeout = base + per_header * (expected number of headers) */
static constexpr int64_t HEADERS_DOWNLOAD_TIMEOUT_BASE = 15 * 60 * 1000000;
static constexpr int64_t HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER = 1000; // 1ms/header
/** Protect at least this many outbound peers from disconnection due to slow/
 * behind headers chain.
 */
static constexpr int32_t MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT = 4;
/** Timeout for (unprotected) outbound peers to sync to our chainwork, in seconds. Allows ~20 blocks to catch up before eviction */
static constexpr int64_t CHAIN_SYNC_TIMEOUT = 300; // def=20m
/** How frequently to check for stale tips, in seconds. Checks for a stale tip every ~10 blocks */
static constexpr int64_t STALE_CHECK_INTERVAL = 150; // def=10m
/** How frequently to check for extra outbound peers and disconnect, in seconds. Prune extra outbound peers roughly every minute, 4 blocks × BT */ /** Peer interval & min con time slightly inversely proportinal to chain sync and stale check! */
static constexpr int64_t EXTRA_PEER_CHECK_INTERVAL = 60; // def=45
/** Minimum time an outbound-peer-eviction candidate must be connected for, in order to evict, in seconds. Ensures peers stay long enough before eviction, 3 blocks × BT */
static constexpr int64_t MINIMUM_CONNECT_TIME = 45; // def=30

/** The maximum rate of address records we're willing to process on average.
 * Is bypassed for whitelisted connections. */
static constexpr double MAX_ADDR_RATE_PER_SECOND{0.1};

/** The soft limit of the address processing token bucket (the regular MAX_ADDR_RATE_PER_SECOND
 *  based increments won't go above this, but the MAX_ADDR_TO_SEND increment following GETADDR
 *  is exempt from this limit. */
static constexpr size_t MAX_ADDR_PROCESSING_TOKEN_BUCKET{MAX_ADDR_TO_SEND};

class PeerLogicValidation : public CValidationInterface, public NetEventsInterface {
private:
    CConnman* const connman;

public:
    explicit PeerLogicValidation(CConnman* connman, CScheduler &scheduler);

    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected, const std::vector<CTransactionRef>& vtxConflicted) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void BlockChecked(const CBlock& block, const CValidationState& state) override;
    void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock) override;


    void InitializeNode(CNode* pnode) override;
    void FinalizeNode(NodeId nodeid, bool& fUpdateConnectionTime) override;
    /** Process protocol messages received from a given node */
    bool ProcessMessages(CNode* pfrom, std::atomic<bool>& interrupt) override;
    /**
    * Send queued protocol messages to be sent to a give node.
    *
    * @param[in]   pto             The node which we are sending messages to.
    * @param[in]   interrupt       Interrupt condition for processing threads
    * @return                      True if there is more work to be done
    */
    bool SendMessages(CNode* pto, std::atomic<bool>& interrupt) override EXCLUSIVE_LOCKS_REQUIRED(pto->cs_sendProcessing);

    void ConsiderEviction(CNode *pto, int64_t time_in_seconds);
    void CheckForStaleTipAndEvictPeers(const Consensus::ConsensusParams &consensusParams);
    void EvictExtraOutboundPeers(int64_t time_in_seconds);

private:
    int64_t m_stale_tip_check_time; //! Next time to check for stale tip
};

struct CNodeStateStats {
    int nMisbehavior;
    int nSyncHeight;
    int nCommonHeight;
    std::vector<int> vHeightInFlight;
};

/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int howmuch);

/** Parse Soteria client version from user agent string */
bool ParseClientVersion(const std::string& userAgent, int& major, int& minor, int& revision);
/** Check if client version is below minimum required (1.0.0) */
bool IsClientVersionBelowMinimum(const std::string& userAgent);

#endif // SOTERIA_NET_PROCESSING_H
