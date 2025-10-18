// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_NET_H
#define SOTERIA_NET_H

#include <algorithm>
#include <vector>
#include <addrdb.h>
#include <addrman.h>
#include <amount.h>
#include <bloom.h>
#include <compat.h>
#include <consensus/params.h>
#include <hash.h>
#include <limitedmap.h>
#include <netaddress.h>
#include <policy/feerate.h>
#include <protocol.h>
#include <random.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <threadinterrupt.h>
#include <string>
#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <list>
#include <stdint.h>
#include <thread>
#include <threadsafety.h>
#include <memory>
#include <functional>
#include <condition_variable>
#ifndef WIN32
#include <arpa/inet.h>
#endif

class CScheduler;
class CNode;
namespace boost {
    class thread_group;
}
/** Time between pings automatically sent out for latency probing and keepalive (in seconds). */
static constexpr int PING_INTERVAL = 90;
/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static constexpr int TIMEOUT_INTERVAL = 12 * 60;
/** Run the feeler connection loop once every 2 minutes or 120 seconds. **/
static constexpr int FEELER_INTERVAL = 90;
/** The maximum number of entries in an 'inv' protocol message. 100 000 entries → 1 message for ~100 000 blocks (~17 days @15 s), 3.6 MiB parsable in < 20 ms */
static constexpr unsigned int MAX_INV_SZ = 100000; //def=50000, in the future 150K
/** The maximum number of entries in a locator */
static constexpr unsigned int MAX_LOCATOR_SZ = 101;
/** The maximum number of entries in an 'asset inv' protocol message */
static constexpr unsigned int MAX_ASSET_INV_SZ = 1024;
/** The maximum number of new addresses to accumulate before announcing. */
static constexpr unsigned int MAX_ADDR_TO_SEND = 1000;
/** Maximum length of incoming protocol messages (no message over 4 MB is currently acceptable in RVN). With our 3 MiB blocks, a raw block message (headers + txs) can exceed 4 MiB once we add envelope headers, varints, padding, or future block‐format extensions. Recommendation, we should raise to at least 5 MiB w/o sc or 6 MiB w sc. 6 MiB safely covers current blocks and leaves headroom for spikes (e.g., “getdata” replies, packed headers, larger sc scripts).*/
static constexpr unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 6 * 1024 * 1024;
/** Maximum length of strSubVer in `version` message */
static constexpr unsigned int MAX_SUBVERSION_LENGTH = 256;
/** Maximum number of automatic outgoing nodes over which we'll relay everything (blocks, tx, addrs, etc), 32 to reduce latency before 90% of peers see a new block */
static constexpr int MAX_OUTBOUND_CONNECTIONS = 32; // 12R, 16nD, 32nV
/** Maximum number of addnode outgoing nodes */
static constexpr int MAX_ADDNODE_CONNECTIONS = 24;
/** -listen default */
static constexpr bool DEFAULT_LISTEN = true;
/** -upnp default */
#ifdef USE_UPNP
static constexpr bool DEFAULT_UPNP = USE_UPNP;
#else
static constexpr bool DEFAULT_UPNP = false;
#endif
/** The maximum number of entries in mapAskFor */
static constexpr size_t MAPASKFOR_MAX_SZ = MAX_INV_SZ;
/** The maximum number of entries in setAskFor (larger due to getdata latency)*/
static constexpr size_t SETASKFOR_MAX_SZ = 2 * MAX_INV_SZ;
/** The maximum number of peer connections to maintain, default unlimited = 125, to override set maxconnections=1024 in coin.conf/coind -help -maxconnections=1024, value depends on hardware cap.*/
static constexpr unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125; // nX 256
/** The default for -maxuploadtarget. 0 = Unlimited */
static constexpr uint64_t DEFAULT_MAX_UPLOAD_TARGET = 0;
/** The default timeframe for -maxuploadtarget. 1 day. */
static constexpr uint64_t MAX_UPLOAD_TIMEFRAME = 60 * 60 * 24;
/** Default for blocks only*/
static constexpr bool DEFAULT_BLOCKSONLY = false;
/** Do not force dns seeds by default*/
static constexpr bool DEFAULT_FORCEDNSSEED = false;
static constexpr size_t DEFAULT_MAXRECEIVEBUFFER = 500 * 1000;
static constexpr size_t DEFAULT_MAXSENDBUFFER    = 100 * 1000;
// NOTE: When adjusting this, update rpcnet:setban's help ("24h")
static constexpr unsigned int DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 3; // as a temp measure until next release
typedef int64_t NodeId;
struct AddedNodeInfo
{
    std::string strAddedNode;
    CService resolvedAddress;
    bool fConnected;
    bool fInbound;
};
class CNodeStats;
class CClientUIInterface;
struct CSerializedNetMsg
{
    CSerializedNetMsg() = default;
    CSerializedNetMsg(CSerializedNetMsg&&) = default;
    CSerializedNetMsg& operator=(CSerializedNetMsg&&) = default;
    // No copying, only moves.
    CSerializedNetMsg(const CSerializedNetMsg& msg) = delete;
    CSerializedNetMsg& operator=(const CSerializedNetMsg&) = delete;

    std::vector<unsigned char> data;
    std::string command;
};
class NetEventsInterface;
class CConnman
{
public:
    enum NumConnections {
        CONNECTIONS_NONE = 0,
        CONNECTIONS_IN = (1U << 0),
        CONNECTIONS_OUT = (1U << 1),
        CONNECTIONS_ALL = (CONNECTIONS_IN | CONNECTIONS_OUT),
    };
    struct Options
    {
        ServiceFlags nLocalServices = NODE_NONE;
        int nMaxConnections = 0;
        int nMaxOutbound = 0;
        int nMaxAddnode = 0;
        int nMaxFeeler = 0;
        int nBestHeight = 0;
        CClientUIInterface* uiInterface = nullptr;
        NetEventsInterface* m_msgproc = nullptr;
        unsigned int nSendBufferMaxSize = 0;
        unsigned int nReceiveFloodSize = 0;
        uint64_t nMaxOutboundTimeframe = 0;
        uint64_t nMaxOutboundLimit = 0;
        std::vector<std::string> vSeedNodes;
        std::vector<CSubNet> vWhitelistedRange;
        std::vector<CService> vBinds, vWhiteBinds;
        bool m_use_addrman_outgoing = true;
        std::vector<std::string> m_specified_outgoing;
        std::vector<std::string> m_added_nodes;
    };
    void Init(const Options& connOptions) {
        nLocalServices = connOptions.nLocalServices;
        nMaxConnections = connOptions.nMaxConnections;
        nMaxOutbound = std::min(connOptions.nMaxOutbound, connOptions.nMaxConnections);
        nMaxAddnode = connOptions.nMaxAddnode;
        nMaxFeeler = connOptions.nMaxFeeler;
        nBestHeight = connOptions.nBestHeight;
        clientInterface = connOptions.uiInterface;
        m_msgproc = connOptions.m_msgproc;
        nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
        nReceiveFloodSize = connOptions.nReceiveFloodSize;
        {
            LOCK(cs_totalBytesSent);
            nMaxOutboundTimeframe = connOptions.nMaxOutboundTimeframe;
            nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
        }
        vWhitelistedRange = connOptions.vWhitelistedRange;
        {
            LOCK(cs_vAddedNodes);
            vAddedNodes = connOptions.m_added_nodes;
        }
    }
    CConnman(uint64_t seed0, uint64_t seed1);
    ~CConnman();
    bool Start(CScheduler& scheduler, const Options& options);
    void Stop();
    void Interrupt();
    bool GetNetworkActive() const { return fNetworkActive; };
    void SetNetworkActive(bool active);
    bool OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant *grantOutbound = nullptr, const char *strDest = nullptr, bool fOneShot = false, bool fFeeler = false, bool manual_connection = false);
    bool CheckIncomingNonce(uint64_t nonce);
    bool ForNode(NodeId id, std::function<bool(CNode* pnode)> func);
    void PushMessage(CNode* pnode, CSerializedNetMsg&& msg);
    template<typename Callable>
    void ForEachNode(Callable&& func)
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };
    template<typename Callable>
    void ForEachNode(Callable&& func) const
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };
    template<typename Callable, typename CallableAfter>
    void ForEachNodeThen(Callable&& pre, CallableAfter&& post)
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (NodeFullyConnected(node))
                pre(node);
        }
        post();
    };
    template<typename Callable, typename CallableAfter>
    void ForEachNodeThen(Callable&& pre, CallableAfter&& post) const
    {
        LOCK(cs_vNodes);
        for (auto&& node : vNodes) {
            if (NodeFullyConnected(node))
                pre(node);
        }
        post();
    };
    // Addrman functions
    size_t GetAddressCount() const;
    void SetServices(const CService &addr, ServiceFlags nServices);
    void MarkAddressGood(const CAddress& addr);
    void AddNewAddresses(const std::vector<CAddress>& vAddr, const CAddress& addrFrom, int64_t nTimePenalty = 0);
    std::vector<CAddress> GetAddresses();
    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    void Ban(const CNetAddr& netAddr, const BanReason& reason, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    void Ban(const CSubNet& subNet, const BanReason& reason, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    void ClearBanned(); // needed for unit testing
    bool IsBanned(CNetAddr ip);
    bool IsBanned(CSubNet subnet);
    bool Unban(const CNetAddr &ip);
    bool Unban(const CSubNet &ip);
    void GetBanned(banmap_t &banmap);
    void SetBanned(const banmap_t &banmap);
    // This allows temporarily exceeding nMaxOutbound, with the goal of finding
    // a peer that is better than all our current peers.
    void SetTryNewOutboundPeer(bool flag);
    bool GetTryNewOutboundPeer();
    // Return the number of outbound peers we have in excess of our target (eg,
    // if we previously called SetTryNewOutboundPeer(true), and have since set
    // to false, we may have extra peers that we wish to disconnect). This may
    // return a value less than (num_outbound_connections - num_outbound_slots)
    // in cases where some outbound connections are not yet fully connected, or
    // not yet fully disconnected.
    int GetExtraOutboundCount();
    bool AddNode(const std::string& node);
    bool RemoveAddedNode(const std::string& node);
    std::vector<AddedNodeInfo> GetAddedNodeInfo();
    size_t GetNodeCount(NumConnections num);
    void GetNodeStats(std::vector<CNodeStats>& vstats);
    bool DisconnectNode(const std::string& node);
    bool DisconnectNode(NodeId id);
    ServiceFlags GetLocalServices() const;
    //!set the max outbound target in bytes
    void SetMaxOutboundTarget(uint64_t limit);
    uint64_t GetMaxOutboundTarget();
    //!set the timeframe for the max outbound target
    void SetMaxOutboundTimeframe(uint64_t timeframe);
    uint64_t GetMaxOutboundTimeframe();
    //!check if the outbound target is reached
    // if param historicalBlockServingLimit is set true, the function will
    // response true if the limit for serving historical blocks has been reached
    bool OutboundTargetReached(bool historicalBlockServingLimit);
    //!response the bytes left in the current max outbound cycle
    // in case of no limit, it will always response 0
    uint64_t GetOutboundTargetBytesLeft();
    //!response the time in second left in the current max outbound cycle
    // in case of no limit, it will always response 0
    uint64_t GetMaxOutboundTimeLeftInCycle();
    uint64_t GetTotalBytesRecv();
    uint64_t GetTotalBytesSent();
    void SetBestHeight(int height);
    int GetBestHeight() const;
    /** Get a unique deterministic randomizer. */
    CSipHasher GetDeterministicRandomizer(uint64_t id) const;
    unsigned int GetReceiveFloodSize() const;
    void WakeMessageHandler();
private:
    struct ListenSocket {
        SOCKET socket;
        bool whitelisted;
        ListenSocket(SOCKET socket_, bool whitelisted_) : socket(socket_), whitelisted(whitelisted_) {}
    };
    bool BindListenPort(const CService &bindAddr, std::string& strError, bool fWhitelisted = false);
    bool Bind(const CService &addr, unsigned int flags);
    bool InitBinds(const std::vector<CService>& binds, const std::vector<CService>& whiteBinds);
    void ThreadOpenAddedConnections();
    void AddOneShot(const std::string& strDest);
    void ProcessOneShot();
    void ThreadOpenConnections(const std::vector<std::string> connect = {});
    void ThreadMessageHandler();
    void AcceptConnection(const ListenSocket& hListenSocket);
    void ThreadSocketHandler();
    void ThreadDNSAddressSeed();
    uint64_t CalculateKeyedNetGroup(const CAddress& ad) const;
    CNode* FindNode(const CNetAddr& ip);
    CNode* FindNode(const CSubNet& subNet);
    CNode* FindNode(const std::string& addrName);
    CNode* FindNode(const CService& addr);
    bool AttemptToEvictConnection();
    CNode* ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure);
    bool IsWhitelistedRange(const CNetAddr &addr);
    void DeleteNode(CNode* pnode);
    NodeId GetNewNodeId();
    size_t SocketSendData(CNode *pnode) const;
    //!check is the banlist has unwritten changes
    bool BannedSetIsDirty();
    //!set the "dirty" flag for the banlist
    void SetBannedSetDirty(bool dirty=true);
    //!clean unused entries (if bantime has expired)
    void SweepBanned();
    void DumpAddresses();
    void DumpData();
    void DumpBanlist();
    // Network stats
    void RecordBytesRecv(uint64_t bytes);
    void RecordBytesSent(uint64_t bytes);
    // Whether the node should be passed out in ForEach* callbacks
    static bool NodeFullyConnected(const CNode* pnode);
    // Network usage totals
    CCriticalSection cs_totalBytesRecv;
    CCriticalSection cs_totalBytesSent;
    uint64_t nTotalBytesRecv GUARDED_BY(cs_totalBytesRecv);
    uint64_t nTotalBytesSent GUARDED_BY(cs_totalBytesSent);
    // outbound limit & stats
    uint64_t nMaxOutboundTotalBytesSentInCycle GUARDED_BY(cs_totalBytesSent);
    uint64_t nMaxOutboundCycleStartTime GUARDED_BY(cs_totalBytesSent);
    uint64_t nMaxOutboundLimit GUARDED_BY(cs_totalBytesSent);
    uint64_t nMaxOutboundTimeframe GUARDED_BY(cs_totalBytesSent);
    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<CSubNet> vWhitelistedRange;
    unsigned int nSendBufferMaxSize{0};
    unsigned int nReceiveFloodSize{0};
    std::vector<ListenSocket> vhListenSocket;
    std::atomic<bool> fNetworkActive{true};
    banmap_t setBanned;
    CCriticalSection cs_setBanned;
    bool setBannedIsDirty;
    bool fAddressesInitialized{false};
    CAddrMan addrman;
    std::deque<std::string> vOneShots GUARDED_BY(cs_vOneShots);
    CCriticalSection cs_vOneShots;
    std::vector<std::string> vAddedNodes GUARDED_BY(cs_vAddedNodes);
    CCriticalSection cs_vAddedNodes;
    std::vector<CNode*> vNodes GUARDED_BY(cs_vNodes);
    std::list<CNode*> vNodesDisconnected;
    mutable CCriticalSection cs_vNodes;
    std::atomic<NodeId> nLastNodeId{0};
    /** Services this instance offers */
    ServiceFlags nLocalServices;
    CSemaphore *semOutbound;
    CSemaphore *semAddnode;
    int nMaxConnections;
    int nMaxOutbound;
    int nMaxAddnode;
    int nMaxFeeler;
    std::atomic<int> nBestHeight;
    CClientUIInterface* clientInterface;
    NetEventsInterface* m_msgproc;
    /** SipHasher seeds for deterministic randomness */
    const uint64_t nSeed0, nSeed1;
    /** flag for waking the message processor. */
    bool fMsgProcWake;
    std::condition_variable condMsgProc;
    std::mutex mutexMsgProc;
    std::atomic<bool> flagInterruptMsgProc{false};
    CThreadInterrupt interruptNet;
    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    std::thread threadMessageHandler;

    /** flag for deciding to connect to an extra outbound peer,
     *  in excess of nMaxOutbound
     *  This takes the place of a feeler connection */
    std::atomic_bool m_try_another_outbound_peer;
};
extern std::unique_ptr<CConnman> g_connman;
void Discover(boost::thread_group& threadGroup);
void MapPort(bool fUseUPnP);
uint16_t GetListenPort();
bool BindListenPort(const CService &bindAddr, std::string& strError, bool fWhitelisted = false);
struct CombinerAll
{
    typedef bool result_type;

    template<typename I>
    bool operator()(I first, I last) const
    {
        while (first != last) {
            if (!(*first)) return false;
            ++first;
        }
        return true;
    }
};
/**Interface for message handling */
class NetEventsInterface
{
public:
    virtual bool ProcessMessages(CNode* pnode, std::atomic<bool>& interrupt) = 0;
    virtual bool SendMessages(CNode* pnode, std::atomic<bool>& interrupt) = 0;
    virtual void InitializeNode(CNode* pnode) = 0;
    virtual void FinalizeNode(NodeId id, bool& update_connection_time) = 0;
protected:
    /**
     * Protected destructor so that instances can only be deleted by derived
     * classes. If that restriction is no longer desired, this should be made
     * public and virtual.
     */
    ~NetEventsInterface() = default;
};
enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_UPNP,   // address reported by UPnP
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};
bool IsPeerAddrLocalGood(CNode *pnode);
void AdvertiseLocal(CNode *pnode);
void SetLimited(enum Network net, bool fLimited = true);
bool IsLimited(enum Network net);
bool IsLimited(const CNetAddr& addr);
bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
bool RemoveLocal(const CService& addr);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
bool GetLocal(CService &addr, const CNetAddr *paddrPeer = nullptr);
bool IsReachable(enum Network net);
bool IsReachable(const CNetAddr &addr);
CAddress GetLocalAddress(const CNetAddr *paddrPeer, ServiceFlags nLocalServices);
extern bool fDiscover;
extern bool fListen;
extern bool fRelayTxes;
extern limitedmap<uint256, int64_t> mapAlreadyAskedFor;
/** Subversion as sent to the P2P network in `version` messages */
extern std::string strSubVersion;
struct LocalServiceInfo {
    int nScore;
    int nPort;
};
extern CCriticalSection cs_mapLocalHost;
extern std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(cs_mapLocalHost);
typedef std::map<std::string, uint64_t> mapMsgCmdSize; //command, total bytes
class CNodeStats
{
public:
    NodeId nodeid;
    ServiceFlags nServices;
    bool fRelayTxes;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    int64_t nTimeOffset;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    bool m_manual_connection;
    int nStartingHeight;
    uint64_t nSendBytes;
    mapMsgCmdSize mapSendBytesPerMsgCmd;
    uint64_t nRecvBytes;
    mapMsgCmdSize mapRecvBytesPerMsgCmd;
    bool fWhitelisted;
    double dPingTime;
    double dPingWait;
    double dMinPing;
    CAmount minFeeFilter;
    // Our address, as reported by the peer
    std::string addrLocal;
    // Address of this peer
    CAddress addr;
    // Bind address of our side of the connection
    CAddress addrBind;
    uint64_t nProcessedAddrs;
    uint64_t nRatelimitedAddrs;
};
class CNetMessage {
private:
    mutable CHash256 hasher;
    mutable uint256 data_hash;
public:
    bool in_data;                   // parsing header (false) or data (true)
    CDataStream hdrbuf;             // partially received header
    CMessageHeader hdr;             // complete header
    unsigned int nHdrPos;
    CDataStream vRecv;              // received message data
    unsigned int nDataPos;
    int64_t nTime;                  // time (in microseconds) of message receipt.
    CNetMessage(const CMessageHeader::MessageStartChars& pchMessageStartIn, int nTypeIn, int nVersionIn) : hdrbuf(nTypeIn, nVersionIn), hdr(pchMessageStartIn), vRecv(nTypeIn, nVersionIn) {
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        nTime = 0;
    }
    bool complete() const
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }
    const uint256& GetMessageHash() const;
    void SetVersion(int nVersionIn)
    {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }

    int readHeader(const char *pch, unsigned int nBytes);
    int readData(const char *pch, unsigned int nBytes);
};
/** Information about a peer */
class CNode
{
    friend class CConnman;
public:
    // socket
    std::atomic<ServiceFlags> nServices{NODE_NONE};
    SOCKET hSocket GUARDED_BY(cs_hSocket);
    size_t nSendSize{0}; // total size of all vSendMsg entries
    size_t nSendOffset{0}; // offset inside the first vSendMsg already sent
    uint64_t nSendBytes GUARDED_BY(cs_vSend){0};
    std::deque<std::vector<unsigned char>> vSendMsg GUARDED_BY(cs_vSend);
    CCriticalSection cs_vSend;
    CCriticalSection cs_hSocket;
    CCriticalSection cs_vRecv;
    CCriticalSection cs_vProcessMsg;
    std::list<CNetMessage> vProcessMsg GUARDED_BY(cs_vProcessMsg);
    size_t nProcessQueueSize{0};
    CCriticalSection cs_sendProcessing;
    std::deque<CInv> vRecvGetData;
    std::deque<CInvAsset> vRecvAssetGetData;
    uint64_t nRecvBytes GUARDED_BY(cs_vRecv){0};
    std::atomic<int> nRecvVersion{INIT_PROTO_VERSION};
    std::atomic<int64_t> nLastSend{0};
    std::atomic<int64_t> nLastRecv{0};
    const int64_t nTimeConnected;
    std::atomic<int64_t> nTimeOffset{0};
    // Address of this peer
    const CAddress addr;
    // Bind address of our side of the connection
    const CAddress addrBind;
    std::atomic<int> nVersion{0};
    // strSubVer is whatever byte array we read from the wire. However, this field is intended
    // to be printed out, displayed to humans in various forms and so on. So we sanitize it and
    // store the sanitized version in cleanSubVer. The original should be used when dealing with
    // the network or wire types and the cleaned string used when displayed or logged.
    std::string strSubVer, cleanSubVer;
    CCriticalSection cs_SubVer; // used for both cleanSubVer and strSubVer
    bool fWhitelisted; // This peer can bypass DoS banning.
    bool fFeeler{false}; // If true this node is being used as a short lived feeler.
    bool fOneShot{false};
    bool m_manual_connection{false};
    bool fClient{false};
    const bool fInbound;
    std::atomic_bool fSuccessfullyConnected{false};
    std::atomic_bool fDisconnect{false};
    // We use fRelayTxes for two purposes -
    // a) it allows us to not relay tx invs before receiving the peer's version message
    // b) the peer may tell us in its version message that we should not relay tx invs
    //    unless it loads a bloom filter.
    bool fRelayTxes{false};
    bool fSentAddr{false};
    CSemaphoreGrant grantOutbound;
    CCriticalSection cs_filter;
    CBloomFilter* pfilter;
    std::atomic<int> nRefCount{0};
    const uint64_t nKeyedNetGroup;
    std::atomic_bool fPauseRecv{false};
    std::atomic_bool fPauseSend{false};
protected:
    mapMsgCmdSize mapSendBytesPerMsgCmd;
    mapMsgCmdSize mapRecvBytesPerMsgCmd GUARDED_BY(cs_vRecv);
public:
    uint256 hashContinue;
    std::atomic<int> nStartingHeight{-1};
    // flood relay
    std::vector<CAddress> vAddrToSend;
    CRollingBloomFilter addrKnown;
    bool fGetAddr{false};
    std::set<uint256> setKnown;
    int64_t nNextAddrSend;
    int64_t nNextLocalAddrSend;
    /** Number of addresses that can be processed from this peer. */
    double nAddrTokenBucket{1};
    /** When nAddrTokenBucket was last updated, in microseconds */
    int64_t nAddrTokenTimestamp;
    std::atomic<uint64_t> nProcessedAddrs{};
    std::atomic<uint64_t> nRatelimitedAddrs{};
    bool fGetAssetData;
    std::set<std::string> setInventoryAssetsSend;
    // inventory based relay
    CRollingBloomFilter filterInventoryKnown GUARDED_BY(cs_inventory);
    // Set of transaction ids we still have to announce.
    // They are sorted by the mempool before relay, so the order is not important.
    std::set<uint256> setInventoryTxToSend;
    // List of block ids we still have announce.
    // There is no final sorting before sending, as they are always sent immediately
    // and in the order requested.
    std::vector<uint256> vInventoryBlockToSend GUARDED_BY(cs_inventory);
    CCriticalSection cs_inventory;
    std::set<uint256> setAskFor;
    std::multimap<int64_t, CInv> mapAskFor;
    int64_t nNextInvSend{0};
    // Used for headers announcements - unfiltered blocks to relay
    // Also protected by cs_inventory
    std::vector<uint256> vBlockHashesToAnnounce GUARDED_BY(cs_inventory);
    // Used for BIP35 mempool sending, also protected by cs_inventory
    bool fSendMempool GUARDED_BY(cs_inventory){false};
    // Last time a "MEMPOOL" request was serviced.
    std::atomic<int64_t> timeLastMempoolReq{0};
    // Block and TXN accept times
    std::atomic<int64_t> nLastBlockTime{0};
    std::atomic<int64_t> nLastTXTime{0};
    // Ping time measurement:
    // The pong reply we're expecting, or 0 if no pong expected.
    std::atomic<uint64_t> nPingNonceSent{0};
    // Time (in usec) the last ping was sent, or 0 if no ping was ever sent.
    std::atomic<int64_t> nPingUsecStart{0};
    // Last measured round-trip time.
    std::atomic<int64_t> nPingUsecTime{0};
    // Best measured round-trip time.
    std::atomic<int64_t> nMinPingUsecTime{std::numeric_limits<int64_t>::max()};
    // Whether a ping is requested.
    std::atomic<bool> fPingQueued{false};
    // Minimum fee rate with which to filter inv's to this node
    CAmount minFeeFilter GUARDED_BY(cs_feeFilter){0};
    CCriticalSection cs_feeFilter;
    CAmount lastSentFeeFilter{0};
    int64_t nextSendTimeFeeFilter{0};
    CNode(NodeId id, ServiceFlags nLocalServicesIn, int nMyStartingHeightIn, SOCKET hSocketIn, const CAddress &addrIn, uint64_t nKeyedNetGroupIn, uint64_t nLocalHostNonceIn, const CAddress &addrBindIn, const std::string &addrNameIn = "", bool fInboundIn = false);
    ~CNode();
    CNode(const CNode&) = delete;
    CNode& operator=(const CNode&) = delete;
private:
    const NodeId id;
    const uint64_t nLocalHostNonce;
    // Services offered to this peer
    const ServiceFlags nLocalServices;
    const int nMyStartingHeight;
    int nSendVersion{0};
    std::list<CNetMessage> vRecvMsg;  // Used only by SocketHandler thread
    mutable CCriticalSection cs_addrName;
    std::string addrName GUARDED_BY(cs_addrName);
    // Our address, as reported by the peer
    CService addrLocal GUARDED_BY(cs_addrLocal);
    mutable CCriticalSection cs_addrLocal;
public:
    NodeId GetId() const {
        return id;
    }
    uint64_t GetLocalNonce() const {
        return nLocalHostNonce;
    }
    int GetMyStartingHeight() const {
        return nMyStartingHeight;
    }
    int GetRefCount() const
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }
    bool ReceiveMsgBytes(const char *pch, unsigned int nBytes, bool& complete);
    void SetRecvVersion(int nVersionIn)
    {
        nRecvVersion = nVersionIn;
    }
    int GetRecvVersion() const
    {
        return nRecvVersion;
    }
    void SetSendVersion(int nVersionIn);
    int GetSendVersion() const;
    CService GetAddrLocal() const;
    //! May not be called more than once
    void SetAddrLocal(const CService& addrLocalIn);
    CNode* AddRef()
    {
        nRefCount++;
        return this;
    }
    void Release()
    {
        nRefCount--;
    }
    void AddAddressKnown(const CAddress& _addr)
    {
        addrKnown.insert(_addr.GetKey());
    }
    void PushAddress(const CAddress& _addr, FastRandomContext &insecure_rand)
    {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        if (_addr.IsValid() && !addrKnown.contains(_addr.GetKey())) {
            if (vAddrToSend.size() >= MAX_ADDR_TO_SEND) {
                vAddrToSend[insecure_rand.randrange(vAddrToSend.size())] = _addr;
            } else {
                vAddrToSend.push_back(_addr);
            }
        }
    }
    void AddInventoryKnown(const CInv& inv)
    {
        {
            LOCK(cs_inventory);
            filterInventoryKnown.insert(inv.hash);
        }
    }
    void PushInventory(const CInv& inv)
    {
        LOCK(cs_inventory);
        if (inv.type == MSG_TX) {
            if (!filterInventoryKnown.contains(inv.hash)) {
                setInventoryTxToSend.insert(inv.hash);
            }
        } else if (inv.type == MSG_BLOCK) {
            vInventoryBlockToSend.push_back(inv.hash);
        }
    }
    void PushAssetInventory(const std::string& name)
    {
        LOCK(cs_inventory);
        setInventoryAssetsSend.insert(name);
    }
    void PushBlockHash(const uint256 &hash)
    {
        LOCK(cs_inventory);
        vBlockHashesToAnnounce.push_back(hash);
    }
    void AskFor(const CInv& inv);
    void CloseSocketDisconnect();
    void copyStats(CNodeStats &stats);
    ServiceFlags GetLocalServices() const
    {
        return nLocalServices;
    }
    std::string GetAddrName() const;
    //! Sets the addrName only if it was not previously set
    void MaybeSetAddrName(const std::string& addrNameIn);
};
class CExplicitNetCleanup
{
public:
    static void callCleanup();
};
/** Return a timestamp in the future (in microseconds) for exponentially distributed events. */
int64_t PoissonNextSend(int64_t nNow, int average_interval_seconds);
#endif // SOTERIA_NET_H
