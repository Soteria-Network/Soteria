// Copyright (c) 2025 The Soteria Core developers

#ifndef SOTER_HASHALGOS_H
#define SOTER_HASHALGOS_H

#include "version.h"
#include "serialize.h"
#include "../../uint256.h"
#include "sph_blake.h"
#include "sph_groestl.h"
#include "sph_jh.h"
#include "sph_keccak.h"
#include "sph_skein.h"
#include "sph_cubehash.h"
#include "sph_simd.h"
#include "sph_echo.h"
#include "sph_hamsi.h"
#include "sph_fugue.h"
#include "sph_shabal.h"
#include "sph_sha2.h"

#include "../../crypto/sha256.h"
#include <vector>

class CBlockHeader;

#ifndef QT_NO_DEBUG
#include <string>
#endif

#ifdef GLOBALDEFINED
#define GLOBAL
#else
#define GLOBAL extern
#endif

inline int GetHashSelection(const uint256& PrevBlockHash, int index) {
    assert(index >= 0 && index < 12); // Preserve bounds check
    constexpr int START = 48;
    constexpr int MASK  = 0xF;
    int pos = START + (index & MASK);
    
    // Fast path: 75-85% of cases
    int nibble = PrevBlockHash.GetNibble(pos);
    if (nibble < 12) return nibble;
    
    // Slow path: search next 15 positions
    for (int i = 1; i < 16; ++i) {
        pos = START + ((index + i) & MASK);
        nibble = PrevBlockHash.GetNibble(pos);
        if (nibble < 12) return nibble;
    }
    
    // Fallback path: mathematically guaranteed to be 0-11
    return PrevBlockHash.GetNibble(pos) % 12;
}

// SHA-256
/** A hasher class for Bitcoin's 256-bit hash (double SHA-256). */
class CHash256 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(hash);
    }

    CHash256& Write(const unsigned char *data, size_t len) {
        sha.Write(data, len);
        return *this;
    }

    CHash256& Reset() {
        sha.Reset();
        return *this;
    }
};

/** Compute the 256-bit hash of an object. */
template<typename T1>
inline uint256 Hash(const T1 pbegin, const T1 pend)
{
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(pbegin == pend ? pblank : (const unsigned char*)&pbegin[0], (pend - pbegin) * sizeof(pbegin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** A writer stream (for serialization) that computes a 256-bit hash. */
class CHashWriter
{
private:
    CHash256 ctx;

public:
    int nType;
    int nVersion;

    CHashWriter(int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn) {}

    int GetType() const { return nType; }
    int GetVersion() const { return nVersion; }

    void write(const char *pch, size_t size) {
        ctx.Write((const unsigned char*)pch, size);
    }

    // invalidates the object
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize((unsigned char*)&result);
        return result;
    }

    template<typename T>
    CHashWriter& operator<<(const T& obj) {
        // Serialize to this stream
        ::Serialize(*this, obj);
        return (*this);
    }
};

/** Compute the 256-bit hash of an object's serialization. */
template<typename T>
uint256 SerializeHash(const T& obj, int nType=SER_GETHASH, int nVersion=PROTOCOL_VERSION)
{
    CHashWriter ss(nType, nVersion);
    ss << obj;
    return ss.GetHash();
}

// SOTERGV1 : 12-hash timestamp-rotated PoW chain
template<typename T1>
inline uint256 HashX12R(const T1 pbegin, const T1 pend, const uint256 PrevBlockHash)
{
    static unsigned char pblank[1];
    uint512 hash[12];

    for (int i = 0; i < 12; i++)
    {
        const void *toHash;
        int lenToHash;

        if (i == 0) {
            toHash = (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0]));
            lenToHash = (pend - pbegin) * sizeof(pbegin[0]);
        } else {
            toHash = static_cast<const void*>(hash[i-1].begin()); // For input
            lenToHash = 64;
        }

        int hashSelection = GetHashSelection(PrevBlockHash, i);

        switch (hashSelection) {
            case 0: {
                sph_blake512_context ctx;
                sph_blake512_init(&ctx);
                sph_blake512(&ctx, toHash, lenToHash);
                sph_blake512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 1: {
                sph_shabal512_context ctx;
                sph_shabal512_init(&ctx);
                sph_shabal512(&ctx, toHash, lenToHash);
                sph_shabal512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 2: {
                sph_groestl512_context ctx;
                sph_groestl512_init(&ctx);
                sph_groestl512(&ctx, toHash, lenToHash);
                sph_groestl512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 3: {
                sph_jh512_context ctx;
                sph_jh512_init(&ctx);
                sph_jh512(&ctx, toHash, lenToHash);
                sph_jh512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 4: {
                sph_keccak512_context ctx;
                sph_keccak512_init(&ctx);
                sph_keccak512(&ctx, toHash, lenToHash);
                sph_keccak512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 5: {
                sph_skein512_context ctx;
                sph_skein512_init(&ctx);
                sph_skein512(&ctx, toHash, lenToHash);
                sph_skein512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 6: {
                sph_fugue512_context ctx;
                sph_fugue512_init(&ctx);
                sph_fugue512(&ctx, toHash, lenToHash);
                sph_fugue512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 7: {
                sph_cubehash512_context ctx;
                sph_cubehash512_init(&ctx);
                sph_cubehash512(&ctx, toHash, lenToHash);
                sph_cubehash512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 8: {
                sph_simd512_context ctx;
                sph_simd512_init(&ctx);
                sph_simd512(&ctx, toHash, lenToHash);
                sph_simd512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 9: {
                sph_echo512_context ctx;
                sph_echo512_init(&ctx);
                sph_echo512(&ctx, toHash, lenToHash);
                sph_echo512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 10: {
                sph_hamsi512_context ctx;
                sph_hamsi512_init(&ctx);
                sph_hamsi512(&ctx, toHash, lenToHash);
                sph_hamsi512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
            case 11: {
                sph_sha512_context ctx;
                sph_sha512_init(&ctx);
                sph_sha512(&ctx, toHash, lenToHash);
                sph_sha512_close(&ctx, static_cast<void*>(hash[i].begin()));
                break;
            }
        }
    }
    return hash[11].trim256();
}

#endif // SOTER_HASHALGOS_H
