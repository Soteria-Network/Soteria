
// Copyright (c) 2011-2017 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-present The Soteria Core developers

#ifndef SOTERIA_RPC_MINING_H
#define SOTERIA_RPC_MINING_H

#include <script/script.h>
#include <primitives/block.h>
#include <memory>
#include <univalue.h>

static const bool DEFAULT_GENERATE = false;
static const int DEFAULT_GENERATE_THREADS = 1;

/** Generate blocks (mine) */
UniValue generateBlocks(std::shared_ptr<CReserveScript> coinbaseScript, int nGenerate, uint64_t nMaxTries, bool keepScript, const POW_TYPE powType);

UniValue getgenerate(const UniValue& params, bool fHelp);

/** Check bounds on a command line confirm target */
unsigned int ParseConfirmTarget(const UniValue& value);

#endif
