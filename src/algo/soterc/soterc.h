// Copyright (c) 2019-2021 The Litecoin Cash Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTER_ALGO_SOTERC_H
#define SOTER_ALGO_SOTERC_H

#include <uint256.h>
#include "algo/soterg/soterg.h"
#include "yespower/yespower.h"
// Config
#define SOTERC_ALGO_COUNT 12
//#define SOTERC_DEBUG

static const yespower_params_t yespower_params = {YESPOWER_1_0, 2048, 8, (const uint8_t*)"et in arcadia ego", 17};

// Graph of hash algos plus SPH contexts
struct Node {
    unsigned int algo;
    Node *childLeft;
    Node *childRight;
};
struct Farm {
    sph_blake512_context context_blake;
    sph_bmw512_context context_bmw;
    sph_cubehash512_context context_cubehash;
    sph_echo512_context context_echo;
    sph_groestl512_context context_groestl;
    sph_hamsi512_context context_hamsi;
    sph_jh512_context context_jh;
    sph_keccak512_context context_keccak;
    sph_luffa512_context context_luffa;
    sph_simd512_context context_simd;
    sph_skein512_context context_skein;
    sph_sha512_context context_sha2;

    Node nodes[22];
};

// Get a 64-byte hash for given 64-byte input, using given Farm contexts and given algo index
uint512 GetHash(uint512 inputHash, Farm *farm, unsigned int algo, yespower_local_t *local) {
    uint512 outputHash;
    switch (algo) {
        case 0:
            sph_blake512_init(&farm->context_blake);
            sph_blake512(&farm->context_blake, static_cast<const void*>(inputHash.begin()), 64);
            sph_blake512_close(&farm->context_blake, static_cast<void*>(outputHash.begin()));
            break;
        case 1:
            sph_bmw512_init(&farm->context_bmw);
            sph_bmw512(&farm->context_bmw, static_cast<const void*>(inputHash.begin()), 64);
            sph_bmw512_close(&farm->context_bmw, static_cast<void*>(outputHash.begin()));        
            break;
        case 2:
            sph_cubehash512_init(&farm->context_cubehash);
            sph_cubehash512(&farm->context_cubehash, static_cast<const void*>(inputHash.begin()), 64);
            sph_cubehash512_close(&farm->context_cubehash, static_cast<void*>(outputHash.begin()));
            break;
        case 3:
            sph_echo512_init(&farm->context_echo);
            sph_echo512(&farm->context_echo, static_cast<const void*>(inputHash.begin()), 64);
            sph_echo512_close(&farm->context_echo, static_cast<void*>(outputHash.begin()));
            break;
        case 4:
            sph_sha512_init(&farm->context_sha2);
            sph_sha512(&farm->context_sha2, static_cast<const void*>(inputHash.begin()), 64);
            sph_sha512_close(&farm->context_sha2, static_cast<void*>(outputHash.begin()));
            break;
        case 5:
            sph_jh512_init(&farm->context_jh);
            sph_jh512(&farm->context_jh, static_cast<const void*>(inputHash.begin()), 64);
            sph_jh512_close(&farm->context_jh, static_cast<void*>(outputHash.begin()));
            break;
        case 6:
            sph_keccak512_init(&farm->context_keccak);
            sph_keccak512(&farm->context_keccak, static_cast<const void*>(inputHash.begin()), 64);
            sph_keccak512_close(&farm->context_keccak, static_cast<void*>(outputHash.begin()));
            break;
        case 7:
            sph_luffa512_init(&farm->context_luffa);
            sph_luffa512(&farm->context_luffa, static_cast<const void*>(inputHash.begin()), 64);
            sph_luffa512_close(&farm->context_luffa, static_cast<void*>(outputHash.begin()));
            break;
        case 8:
            sph_groestl512_init(&farm->context_groestl);
            sph_groestl512(&farm->context_groestl, static_cast<const void*>(inputHash.begin()), 64);
            sph_groestl512_close(&farm->context_groestl, static_cast<void*>(outputHash.begin()));
            break;
        case 9:
            sph_simd512_init(&farm->context_simd);
            sph_simd512(&farm->context_simd, static_cast<const void*>(inputHash.begin()), 64);
            sph_simd512_close(&farm->context_simd, static_cast<void*>(outputHash.begin()));
            break;
        case 10:
            sph_skein512_init(&farm->context_skein);
            sph_skein512(&farm->context_skein, static_cast<const void*>(inputHash.begin()), 64);
            sph_skein512_close(&farm->context_skein, static_cast<void*>(outputHash.begin()));
            break;
        case 11:
            sph_hamsi512_init(&farm->context_hamsi);
            sph_hamsi512(&farm->context_hamsi, static_cast<const void*>(inputHash.begin()), 64);
            sph_hamsi512_close(&farm->context_hamsi, static_cast<void*>(outputHash.begin()));
            break;
        // NB: The CPU-hard gate must be case SOTERC_ALGO_COUNT.
        case 12:
            if (local == NULL)  // Self-manage storage on current thread
                yespower_tls(inputHash.begin(), 64, &yespower_params, (yespower_binary_t*)outputHash.begin());
            else                // Use provided thread-local storage
                yespower(local, inputHash.begin(), 64, &yespower_params, (yespower_binary_t*)outputHash.begin());
            break;
        default:
            assert(false);
            break;
    }
    return outputHash;
}

// Recursively traverse a given farm starting with a given hash and given node within the farm. The hash is overwritten with the final hash.
uint512 TraverseFarm(Farm *farm, uint512 hash, Node *node, yespower_local_t *local) {
    uint512 partialHash = GetHash(hash, farm, node->algo, local);

#ifdef SOTERC_DEBUG
    printf("* Ran algo %d. Partial hash:\t%s\n", node->algo, partialHash.ToString().c_str());
    fflush(0);
#endif

    if (partialHash.ByteAt(63) % 2 == 0) {      // Last byte of output hash is even
        if (node->childLeft != NULL)
            return TraverseFarm(farm, partialHash, node->childLeft, local);
    } else {                                    // Last byte of output hash is odd
        if (node->childRight != NULL)
            return TraverseFarm(farm, partialHash, node->childRight, local);
    }
    return partialHash;
}

// Associate child nodes with a parent node
void LinkNodes(Node *parent, Node *childLeft, Node *childRight) {
    parent->childLeft = childLeft;
    parent->childRight = childRight;
}

// Produce a Soterc 32-byte hash from variable length data
// Optionally, use the SoterC hardened hash.
// Optionally, use provided thread-local memory for yespower.
template<typename T> uint256 Soterc(const T begin, const T end, bool soterC, yespower_local_t *local = NULL) {
    // Create farm nodes. Note that both sides of 19 and 20 lead to 21, and 21 has no children (to make traversal complete).
    // Every path through the farm stops at 7 nodes.
    Farm farm;
    LinkNodes(&farm.nodes[0], &farm.nodes[1], &farm.nodes[2]);
    LinkNodes(&farm.nodes[1], &farm.nodes[3], &farm.nodes[4]);
    LinkNodes(&farm.nodes[2], &farm.nodes[5], &farm.nodes[6]);
    LinkNodes(&farm.nodes[3], &farm.nodes[7], &farm.nodes[8]);
    LinkNodes(&farm.nodes[4], &farm.nodes[9], &farm.nodes[10]);
    LinkNodes(&farm.nodes[5], &farm.nodes[11], &farm.nodes[12]);
    LinkNodes(&farm.nodes[6], &farm.nodes[13], &farm.nodes[14]);
    LinkNodes(&farm.nodes[7], &farm.nodes[15], &farm.nodes[16]);
    LinkNodes(&farm.nodes[8], &farm.nodes[15], &farm.nodes[16]);
    LinkNodes(&farm.nodes[9], &farm.nodes[15], &farm.nodes[16]);
    LinkNodes(&farm.nodes[10], &farm.nodes[15], &farm.nodes[16]);
    LinkNodes(&farm.nodes[11], &farm.nodes[17], &farm.nodes[18]);
    LinkNodes(&farm.nodes[12], &farm.nodes[17], &farm.nodes[18]);
    LinkNodes(&farm.nodes[13], &farm.nodes[17], &farm.nodes[18]);
    LinkNodes(&farm.nodes[14], &farm.nodes[17], &farm.nodes[18]);
    LinkNodes(&farm.nodes[15], &farm.nodes[19], &farm.nodes[20]);
    LinkNodes(&farm.nodes[16], &farm.nodes[19], &farm.nodes[20]);
    LinkNodes(&farm.nodes[17], &farm.nodes[19], &farm.nodes[20]);
    LinkNodes(&farm.nodes[18], &farm.nodes[19], &farm.nodes[20]);
    LinkNodes(&farm.nodes[19], &farm.nodes[21], &farm.nodes[21]);
    LinkNodes(&farm.nodes[20], &farm.nodes[21], &farm.nodes[21]);
    farm.nodes[21].childLeft = NULL;
    farm.nodes[21].childRight = NULL;
        
    // Find initial sha512 hash of the variable length data
    uint512 hash;
    static unsigned char empty[1];
    sph_sha512_init(&farm.context_sha2);
    sph_sha512(&farm.context_sha2, (begin == end ? empty : static_cast<const void*>(&begin[0])), (end - begin) * sizeof(begin[0]));
    sph_sha512_close(&farm.context_sha2, static_cast<void*>(hash.begin()));

#ifdef SOTERC_DEBUG
    printf("** Initial hash:\t\t%s\n", hash.ToString().c_str());
    fflush(0);
#endif    

    // Assign algos to net nodes based on initial hash
    for (int i = 0; i < 22; i++)
        farm.nodes[i].algo = hash.ByteAt(i) % SOTERC_ALGO_COUNT;

    // Hardened farm gates on soterC
    if (soterC)
        farm.nodes[21].algo = SOTERC_ALGO_COUNT;

    // Send the initial hash through the farm
    hash = TraverseFarm(&farm, hash, &farm.nodes[0], local);

#ifdef SOTERC_DEBUG
    printf("** Soterc Final hash:\t\t\t%s\n", uint256(hash).ToString().c_str());
    fflush(0);
#endif

    // Return truncated result
    return uint256(hash);
}

#endif // SOTER_ALGO_SOTERC_H
