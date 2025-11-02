// Copyright (c) 2019-2021 The Litecoin Cash Core developers
// Copyright (c) 2025 The Soteria Core developers

#ifndef SOTER_ALGO_SOTERC_H
#define SOTER_ALGO_SOTERC_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <uint256.h>
#include "algo/soterg/soterg.h"
#include "yespower/yespower.h"

#define SOTERC_ALGO_COUNT 12

static const yespower_params_t yespower_params = {YESPOWER_1_0, 2048, 8, (const uint8_t*)"et in arcadia ego", 17};

// Graph of hash algos plus SPH contexts
struct Node {
    unsigned int algo;
    Node *childLeft;
    Node *childRight;
};

struct Farm {
    sph_blake512_context context_blake;
    sph_shabal512_context context_shabal;
    sph_cubehash512_context context_cubehash;
    sph_echo512_context context_echo;
    sph_groestl512_context context_groestl;
    sph_hamsi512_context context_hamsi;
    sph_jh512_context context_jh;
    sph_keccak512_context context_keccak;
    sph_fugue512_context context_fugue;
    sph_simd512_context context_simd;
    sph_skein512_context context_skein;
    sph_sha512_context context_sha2;

    Node nodes[22];
};

// Node linking helper
inline void LinkNodes(Node *parent, Node *childLeft, Node *childRight) {
    parent->childLeft = childLeft;
    parent->childRight = childRight;
}

// per-algo helpers
static inline void run_blake(Farm* f, const void* in, void* out) {
    sph_blake512_init(&f->context_blake);
    sph_blake512(&f->context_blake, in, 64);
    sph_blake512_close(&f->context_blake, out);
}
static inline void run_shabal(Farm* f, const void* in, void* out) {
    sph_shabal512_init(&f->context_shabal);
    sph_shabal512(&f->context_shabal, in, 64);
    sph_shabal512_close(&f->context_shabal, out);
}
static inline void run_cubehash(Farm* f, const void* in, void* out) {
    sph_cubehash512_init(&f->context_cubehash);
    sph_cubehash512(&f->context_cubehash, in, 64);
    sph_cubehash512_close(&f->context_cubehash, out);
}
static inline void run_echo(Farm* f, const void* in, void* out) {
    sph_echo512_init(&f->context_echo);
    sph_echo512(&f->context_echo, in, 64);
    sph_echo512_close(&f->context_echo, out);
}
static inline void run_sha512(Farm* f, const void* in, void* out) {
    sph_sha512_init(&f->context_sha2);
    sph_sha512(&f->context_sha2, in, 64);
    sph_sha512_close(&f->context_sha2, out);
}
static inline void run_jh(Farm* f, const void* in, void* out) {
    sph_jh512_init(&f->context_jh);
    sph_jh512(&f->context_jh, in, 64);
    sph_jh512_close(&f->context_jh, out);
}
static inline void run_keccak(Farm* f, const void* in, void* out) {
    sph_keccak512_init(&f->context_keccak);
    sph_keccak512(&f->context_keccak, in, 64);
    sph_keccak512_close(&f->context_keccak, out);
}
static inline void run_fugue(Farm* f, const void* in, void* out) {
    sph_fugue512_init(&f->context_fugue);
    sph_fugue512(&f->context_fugue, in, 64);
    sph_fugue512_close(&f->context_fugue, out);
}
static inline void run_groestl(Farm* f, const void* in, void* out) {
    sph_groestl512_init(&f->context_groestl);
    sph_groestl512(&f->context_groestl, in, 64);
    sph_groestl512_close(&f->context_groestl, out);
}
static inline void run_simd(Farm* f, const void* in, void* out) {
    sph_simd512_init(&f->context_simd);
    sph_simd512(&f->context_simd, in, 64);
    sph_simd512_close(&f->context_simd, out);
}
static inline void run_skein(Farm* f, const void* in, void* out) {
    sph_skein512_init(&f->context_skein);
    sph_skein512(&f->context_skein, in, 64);
    sph_skein512_close(&f->context_skein, out);
}
static inline void run_hamsi(Farm* f, const void* in, void* out) {
    sph_hamsi512_init(&f->context_hamsi);
    sph_hamsi512(&f->context_hamsi, in, 64);
    sph_hamsi512_close(&f->context_hamsi, out);
}

// Replace existing GetHash_Write implementation with this corrected version
inline void GetHash_Write(const uint512& inputHash, Farm* farm, unsigned int algo, yespower_local_t* local, uint512& out) {
    // use the exact types yespower expects
    const uint8_t* in = reinterpret_cast<const uint8_t*>(inputHash.begin());
    void* outp = static_cast<void*>(out.begin());

    switch (algo) {
        case 0: run_blake(farm, in, outp); break;
        case 1: run_shabal(farm, in, outp); break;
        case 2: run_cubehash(farm, in, outp); break;
        case 3: run_echo(farm, in, outp); break;
        case 4: run_sha512(farm, in, outp); break;
        case 5: run_jh(farm, in, outp); break;
        case 6: run_keccak(farm, in, outp); break;
        case 7: run_fugue(farm, in, outp); break;
        case 8: run_groestl(farm, in, outp); break;
        case 9: run_simd(farm, in, outp); break;
        case 10: run_skein(farm, in, outp); break;
        case 11: run_hamsi(farm, in, outp); break;
        case SOTERC_ALGO_COUNT:
            if (local == nullptr)
                yespower_tls(in, 64, &yespower_params, reinterpret_cast<yespower_binary_t*>(outp));
            else
                yespower(local, in, 64, &yespower_params, reinterpret_cast<yespower_binary_t*>(outp));
            break;
        default:
            assert(false);
            break;
    }
}
// Efficient traversal (reuse buffer)
inline void TraverseFarm_Ref(Farm* farm, uint512& hash, Node* node, yespower_local_t* local) {
    uint512 partial;
    GetHash_Write(hash, farm, node->algo, local, partial);

#ifdef SOTERC_DEBUG
    printf("* Ran algo %d. Partial hash:\t%s\n", node->algo, partial.ToString().c_str());
    fflush(0);
#endif

    if ((partial.ByteAt(63) & 1) == 0) {
        if (node->childLeft) {
            hash = partial;
            TraverseFarm_Ref(farm, hash, node->childLeft, local);
            return;
        }
    } else {
        if (node->childRight) {
            hash = partial;
            TraverseFarm_Ref(farm, hash, node->childRight, local);
            return;
        }
    }
    hash = partial;
}

// Worker farm lifecycle helpers
static inline Farm* WorkerFarm_Create() {
    Farm* f = static_cast<Farm*>(malloc(sizeof(Farm)));
    if (!f) return nullptr;
    memset(f, 0, sizeof(*f));
    return f;
}
static inline void WorkerFarm_Destroy(Farm* f) {
    if (!f) return;
    free(f);
}

// Soterc variant that accepts worker farm explicitly (hot path)
template<typename T> uint256 Soterc_Worker(const T begin, const T end, bool soterC, Farm* farm, yespower_local_t* local = nullptr) {
    assert(farm != nullptr);

    // init nodes (cheap)
    for (int i = 0; i < 22; ++i) {
        farm->nodes[i].childLeft = nullptr;
        farm->nodes[i].childRight = nullptr;
        farm->nodes[i].algo = 0;
    }

    // wiring
    LinkNodes(&farm->nodes[0], &farm->nodes[1], &farm->nodes[2]);
    LinkNodes(&farm->nodes[1], &farm->nodes[3], &farm->nodes[4]);
    LinkNodes(&farm->nodes[2], &farm->nodes[5], &farm->nodes[6]);
    LinkNodes(&farm->nodes[3], &farm->nodes[7], &farm->nodes[8]);
    LinkNodes(&farm->nodes[4], &farm->nodes[9], &farm->nodes[10]);
    LinkNodes(&farm->nodes[5], &farm->nodes[11], &farm->nodes[12]);
    LinkNodes(&farm->nodes[6], &farm->nodes[13], &farm->nodes[14]);
    LinkNodes(&farm->nodes[7], &farm->nodes[15], &farm->nodes[16]);
    LinkNodes(&farm->nodes[8], &farm->nodes[15], &farm->nodes[16]);
    LinkNodes(&farm->nodes[9], &farm->nodes[15], &farm->nodes[16]);
    LinkNodes(&farm->nodes[10], &farm->nodes[15], &farm->nodes[16]);
    LinkNodes(&farm->nodes[11], &farm->nodes[17], &farm->nodes[18]);
    LinkNodes(&farm->nodes[12], &farm->nodes[17], &farm->nodes[18]);
    LinkNodes(&farm->nodes[13], &farm->nodes[17], &farm->nodes[18]);
    LinkNodes(&farm->nodes[14], &farm->nodes[17], &farm->nodes[18]);
    LinkNodes(&farm->nodes[15], &farm->nodes[19], &farm->nodes[20]);
    LinkNodes(&farm->nodes[16], &farm->nodes[19], &farm->nodes[20]);
    LinkNodes(&farm->nodes[17], &farm->nodes[19], &farm->nodes[20]);
    LinkNodes(&farm->nodes[18], &farm->nodes[19], &farm->nodes[20]);
    LinkNodes(&farm->nodes[19], &farm->nodes[21], &farm->nodes[21]);
    LinkNodes(&farm->nodes[20], &farm->nodes[21], &farm->nodes[21]);

    farm->nodes[21].childLeft = nullptr;
    farm->nodes[21].childRight = nullptr;

    // initial sha512
    uint512 hash;
    static unsigned char empty[1];
    const void* dataPtr = (begin == end ? empty : static_cast<const void*>(&begin[0]));
    size_t dataLen = (end - begin) * sizeof(begin[0]);

    sph_sha512_init(&farm->context_sha2);
    sph_sha512(&farm->context_sha2, dataPtr, dataLen);
    sph_sha512_close(&farm->context_sha2, static_cast<void*>(hash.begin()));

    for (int i = 0; i < 22; ++i) farm->nodes[i].algo = hash.ByteAt(i) % SOTERC_ALGO_COUNT;
    if (soterC) farm->nodes[21].algo = SOTERC_ALGO_COUNT;

    yespower_local_t local_storage;
    yespower_local_t* use_local = local ? local : &local_storage;

    TraverseFarm_Ref(farm, hash, &farm->nodes[0], use_local);

    return uint256(hash);
}

// Backwards-compatible wrapper: creates and destroys a temporary farm (not for hot path)
template<typename T> uint256 Soterc(const T begin, const T end, bool soterC, yespower_local_t* local = nullptr) {
    Farm* f = WorkerFarm_Create();
    if (!f) abort();
    uint256 r = Soterc_Worker(begin, end, soterC, f, local);
    WorkerFarm_Destroy(f);
    return r;
}

#endif
