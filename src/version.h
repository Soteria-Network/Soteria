// Copyright (c) 2012-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_VERSION_H
#define SOTERIA_VERSION_H

/**
 * network protocol versioning
 */

static constexpr int PROTOCOL_VERSION = 70035;

//! initial proto version, to be increased after version/verack negotiation
static constexpr int INIT_PROTO_VERSION = 209;

//! In this version, 'getheaders' was introduced.
static constexpr int GETHEADERS_VERSION = 31800;

//! assetdata network request is allowed for this version
static constexpr int ASSETDATA_VERSION = 70017;

//! soterg + soterc algo
static constexpr int DUALALGO_VERSION = 70032;

//! asset activation
static constexpr int ASSETS_VERSION = 70033;

//! disconnect from peers older than this proto version
static constexpr int MIN_PEER_PROTO_VERSION = DUALALGO_VERSION;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static constexpr int CADDR_TIME_VERSION = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static constexpr int BIP0031_VERSION = 60000;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static constexpr int NO_BLOOM_VERSION = 70011;

//! "sendheaders" command and announcing blocks with headers starts with this version
static constexpr int SENDHEADERS_VERSION = 70012;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
static constexpr int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
static constexpr int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
static constexpr int INVALID_CB_NO_BAN_VERSION = 70015;

//! getassetdata reutrn asstnotfound, and assetdata doesn't have blockhash in the data
static constexpr int ASSETDATA_VERSION_UPDATED = 70020;

//! getassetdata return asstnotfound, and assetdata doesn't have blockhash in the data
static constexpr int X12RV2_VERSION = 70025;

#endif // SOTERIA_VERSION_H
