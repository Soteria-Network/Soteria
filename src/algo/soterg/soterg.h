// Copyright (c) 2025 The Soteria Core developers

#ifndef SOTER_HASHALGOS_H
#define SOTER_HASHALGOS_H

#include "version.h"
#include "serialize.h"
#include "../../uint256.h"
#include "sph_blake.h"
#include "sph_bmw.h"
#include "sph_groestl.h"
#include "sph_jh.h"
#include "sph_keccak.h"
#include "sph_skein.h"
#include "sph_luffa.h"
#include "sph_cubehash.h"
#include "sph_simd.h"
#include "sph_echo.h"
#include "sph_hamsi.h"
#include "sph_sha2.h"
#include "sph_shavite.h"
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
    int hashSelection;

    sph_blake512_context     ctx_blake;      //0 2.1 J/Mhash
    sph_keccak512_context    ctx_keccak;     //1 2.5 J/Mhash
    sph_skein512_context     ctx_skein;      //2 3.2 J/Mhash
    sph_luffa512_context     ctx_luffa;      //3 3.8 J/Mhash 
    sph_cubehash512_context  ctx_cubehash;   //4 4.5 J/Mhash
    sph_simd512_context      ctx_simd;       //5 5.1 J/Mhash
    sph_hamsi512_context     ctx_hamsi;      //6 5.9 J/Mhash
    sph_shavite512_context   ctx_shavite;    //7 7.2 J/Mhash
    sph_jh512_context        ctx_jh;         //8 7.8 J/Mhash
    sph_bmw512_context       ctx_bmw;        //9 8.4 J/Mhash
    sph_groestl512_context   ctx_groestl;    //A 9.1 J/Mhash
    sph_echo512_context      ctx_echo;       //B 10.3 J/Mhash      

    static unsigned char pblank[1];

    uint512 hash[12];

    for (int i=0;i<12;i++)
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

        hashSelection = GetHashSelection(PrevBlockHash, i);
/** We should avoid relying on the memory layout of the class (even though it's simple and without virtual functions). We explicitly state that we are passing the internal data buffer in each case */
        switch(hashSelection) {
            case 0:
                sph_blake512_init(&ctx_blake);
                sph_blake512 (&ctx_blake, toHash, lenToHash); 
                sph_blake512_close(&ctx_blake, static_cast<void*>(hash[i].begin())); // For output
                break;
            case 1:
                sph_bmw512_init(&ctx_bmw);
                sph_bmw512 (&ctx_bmw, toHash, lenToHash);
                sph_bmw512_close(&ctx_bmw, static_cast<void*>(hash[i].begin()));
                break;
            case 2:
                sph_groestl512_init(&ctx_groestl);
                sph_groestl512 (&ctx_groestl, toHash, lenToHash);
                sph_groestl512_close(&ctx_groestl, static_cast<void*>(hash[i].begin()));
                break;
            case 3:
                sph_jh512_init(&ctx_jh);
                sph_jh512 (&ctx_jh, toHash, lenToHash);
                sph_jh512_close(&ctx_jh, static_cast<void*>(hash[i].begin()));
                break;
            case 4:
                sph_keccak512_init(&ctx_keccak);
                sph_keccak512 (&ctx_keccak, toHash, lenToHash);
                sph_keccak512_close(&ctx_keccak, static_cast<void*>(hash[i].begin()));
                break;
            case 5:
                sph_skein512_init(&ctx_skein);
                sph_skein512 (&ctx_skein, toHash, lenToHash);
                sph_skein512_close(&ctx_skein, static_cast<void*>(hash[i].begin()));
                break;
            case 6:
                sph_luffa512_init(&ctx_luffa);
                sph_luffa512 (&ctx_luffa, toHash, lenToHash);
                sph_luffa512_close(&ctx_luffa, static_cast<void*>(hash[i].begin()));
                break;
            case 7:
                sph_cubehash512_init(&ctx_cubehash);
                sph_cubehash512 (&ctx_cubehash, toHash, lenToHash);
                sph_cubehash512_close(&ctx_cubehash, static_cast<void*>(hash[i].begin()));
                break;
            case 8:
                sph_simd512_init(&ctx_simd);
                sph_simd512 (&ctx_simd, toHash, lenToHash);
                sph_simd512_close(&ctx_simd, static_cast<void*>(hash[i].begin()));
                break;
            case 9:
                sph_echo512_init(&ctx_echo);
                sph_echo512 (&ctx_echo, toHash, lenToHash);
                sph_echo512_close(&ctx_echo, static_cast<void*>(hash[i].begin()));
                break;
            case 10:
                sph_hamsi512_init(&ctx_hamsi);
                sph_hamsi512 (&ctx_hamsi, toHash, lenToHash);
                sph_hamsi512_close(&ctx_hamsi, static_cast<void*>(hash[i].begin()));
                break;
            case 11:
                sph_shavite512_init(&ctx_shavite);
                sph_shavite512(&ctx_shavite, toHash, lenToHash);
                sph_shavite512_close(&ctx_shavite, static_cast<void*>(hash[i].begin()));
                break;
        }
    }
    return hash[11].trim256();
}

#endif // SOTER_HASHALGOS_H
