// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/powcache.h>
#include <primitives/block.h>
#include <flat-database.h>
#include <sync.h>
#include "../util/system.h"
#include <functional>
#include <sstream>
#include <string>
#include <util/system.h>

CCriticalSection cs_pow;

CPowCache* CPowCache::instance = nullptr;

CPowCache& CPowCache::Instance()
{
    if (CPowCache::instance == nullptr)
    {
        int powCacheSize = gArgs.GetArg("-powhashcache", DEFAULT_POW_CACHE_SIZE);
        bool powCacheValidate = gArgs.GetArg("-powcachevalidate", 0) > 0 ? true : false;
        powCacheSize = powCacheSize == 0 ? DEFAULT_POW_CACHE_SIZE : powCacheSize;

        CPowCache::instance = new CPowCache(powCacheSize, powCacheValidate);
    }
    return *instance;
}

void CPowCache::DoMaintenance()
{
    LOCK(cs_pow);
    // If cache has grown enough, save it:
    if (cacheMap.size() - nLoadedSize > 100)
    {
        CFlatDB<CPowCache> flatDb("powcache.dat", "powCache");
        flatDb.Dump(*this);
    }
}

CPowCache::CPowCache(int maxSize, bool validate) : unordered_lru_cache<uint256, uint256, std::hash<uint256>>(maxSize),
   nVersion(CURRENT_VERSION),
   nLoadedSize(0),
   bValidate(validate)
{
    if (bValidate) LogPrintf("PowCache: Validation and auto correction enabled\n");
}

CPowCache::~CPowCache()
{
}

void CPowCache::Clear()
{
   cacheMap.clear();
}

void CPowCache::CheckAndRemove()
{
}

std::string CPowCache::ToString() const
{
    std::ostringstream info;
    info << "PowCache: elements: " << (int)cacheMap.size();
    return info.str();
}
