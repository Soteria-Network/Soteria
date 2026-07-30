// Copyright (c) 2019-2021 The Litecoin Cash Core developers
// Copyright (c) 2025-2026 The Soteria Core Developers

#ifndef SOTER_ALGO_SOTERC_H
#define SOTER_ALGO_SOTERC_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <uint256.h>
#include "version.h"
#include "serialize.h"
#include "../../uint256.h"
#include "sph_blake.h"
#include "sph_bmw.h"
#include "sph_groestl.h"
#include "sph_jh.h"
#include "sph_keccak.h"
#include "sph_skein.h"
#include "sph_cubehash.h"
#include "sph_simd.h"
#include "sph_echo.h"
#include "sph_hamsi.h"
#include "sph_luffa.h"
#include "sph_shabal.h"
#include "sph_sha2.h"
#include "sph_luffa.h"
#include "sph_fugue.h"
#include "sph_shavite.h"
#include "sph_whirlpool.h"
#include "../../crypto/sha256.h"
#include "yespower/yespower.h"
#include "algo/soterg/soterg.h"
// ====================================================================
// CONFIGURATION
// ====================================================================
#define SOTERC_ALGO_COUNT 8   
#define SOTERC_TREE_NODES 22  

static const yespower_params_t yespower_params = {
    YESPOWER_1_0,                       // version
    2048,                               // N
    8,                                  // r
    (const uint8_t*)"soteria core",     // personalisation string
    13                                  // length
};

// Graph of hash algos plus SPH contexts
struct SotercNode {
    unsigned int algo;
    SotercNode* childLeft;
    SotercNode* childRight;
};

struct SotercGarden {
    sph_cubehash512_context context_cubehash;
    sph_echo512_context context_echo;
    sph_fugue512_context context_fugue;
    sph_hamsi512_context context_hamsi;
    sph_jh512_context context_jh;
    sph_luffa512_context context_luffa;
    sph_shavite512_context context_shavite;
    sph_whirlpool_context context_whirlpool;
    sph_sha512_context context_sha2;

    SotercNode nodes[22];
};

// Get a 64-byte hash for given 64-byte input, using given SotercGarden contexts and given algo index
uint512 Soterc(uint512 inputHash, SotercGarden* garden, unsigned int algo, yespower_local_t* local)
{
    uint512 outputHash;
    switch (algo) {
    // Our 8 efficient algorithms (mapped to 0-7)
    case 0: // FUGUE
        sph_fugue512_init(&garden->context_fugue);
        sph_fugue512(&garden->context_fugue, static_cast<const void*>(&inputHash), 64);
        sph_fugue512_close(&garden->context_fugue, static_cast<void*>(&outputHash));
        break;
    case 1: // WHIRLPOOL
        sph_whirlpool_init(&garden->context_whirlpool);
        sph_whirlpool(&garden->context_whirlpool, static_cast<const void*>(&inputHash), 64);
        sph_whirlpool_close(&garden->context_whirlpool, static_cast<void*>(&outputHash));
        break;
    case 2: // ECHO
        sph_echo512_init(&garden->context_echo);
        sph_echo512(&garden->context_echo, static_cast<const void*>(&inputHash), 64);
        sph_echo512_close(&garden->context_echo, static_cast<void*>(&outputHash));
        break;
    case 3: // LUFFA
        sph_luffa512_init(&garden->context_luffa);
        sph_luffa512(&garden->context_luffa, static_cast<const void*>(&inputHash), 64);
        sph_luffa512_close(&garden->context_luffa, static_cast<void*>(&outputHash));
        break;
    case 4: // SHAVITE
        sph_shavite512_init(&garden->context_shavite);
        sph_shavite512(&garden->context_shavite, static_cast<const void*>(&inputHash), 64);
        sph_shavite512_close(&garden->context_shavite, static_cast<void*>(&outputHash));
        break;
    case 5: // HAMSI
        sph_hamsi512_init(&garden->context_hamsi);
        sph_hamsi512(&garden->context_hamsi, static_cast<const void*>(&inputHash), 64);
        sph_hamsi512_close(&garden->context_hamsi, static_cast<void*>(&outputHash));
        break;
    case 6: // JH
        sph_jh512_init(&garden->context_jh);
        sph_jh512(&garden->context_jh, static_cast<const void*>(&inputHash), 64);
        sph_jh512_close(&garden->context_jh, static_cast<void*>(&outputHash));
        break;
    case 7: // CUBEHASH
        sph_cubehash512_init(&garden->context_cubehash);
        sph_cubehash512(&garden->context_cubehash, static_cast<const void*>(&inputHash), 64);
        sph_cubehash512_close(&garden->context_cubehash, static_cast<void*>(&outputHash));
        break;
    // NB: The CPU-hard gate must be case SOTERC_ALGO_COUNT (8).
    case 8:
        if (local == NULL) // Self-manage storage on current thread
            yespower_tls(inputHash.begin(), 64, &yespower_params, (yespower_binary_t*)outputHash.begin());
        else // Use provided thread-local storage
            yespower(local, inputHash.begin(), 64, &yespower_params, (yespower_binary_t*)outputHash.begin());

        break;
    default:
        assert(false);
        break;
    }

    return outputHash;
}

// Recursively traverse a given garden starting with a given hash and given node within the garden. The hash is overwritten with the final hash.
uint512 TraverseSotercGarden(SotercGarden* garden, uint512 hash, SotercNode* node, yespower_local_t* local)
{
    uint512 partialHash = Soterc(hash, garden, node->algo, local);

#ifdef SOTERC_DEBUG
    printf("* Ran algo %d. Partial hash:\t%s\n", node->algo, partialHash.ToString().c_str());
    fflush(0);
#endif

    if (partialHash.ByteAt(63) % 2 == 0) { // Last byte of output hash is even
        if (node->childLeft != NULL)
            return TraverseSotercGarden(garden, partialHash, node->childLeft, local);
    } else { // Last byte of output hash is odd
        if (node->childRight != NULL)
            return TraverseSotercGarden(garden, partialHash, node->childRight, local);
    }

    return partialHash;
}

// Associate child nodes with a parent node
void LinkSotercNodes(SotercNode* parent, SotercNode* childLeft, SotercNode* childRight)
{
    parent->childLeft = childLeft;
    parent->childRight = childRight;
}

// Produce a Soterc 32-byte hash from variable length data
// Optionally, use the Soterc hardened hash.
// Optionally, use provided thread-local memory for yespower.
template <typename T>
uint256 Soterc(const T begin, const T end, bool sotercHardened = true, yespower_local_t* local = NULL)
{
    // Create garden nodes. Note that both sides of 19 and 20 lead to 21, and 21 has no children (to make traversal complete).
    // Every path through the garden stops at 7 nodes.
    SotercGarden garden;
    LinkSotercNodes(&garden.nodes[0], &garden.nodes[1], &garden.nodes[2]);
    LinkSotercNodes(&garden.nodes[1], &garden.nodes[3], &garden.nodes[4]);
    LinkSotercNodes(&garden.nodes[2], &garden.nodes[5], &garden.nodes[6]);
    LinkSotercNodes(&garden.nodes[3], &garden.nodes[7], &garden.nodes[8]);
    LinkSotercNodes(&garden.nodes[4], &garden.nodes[9], &garden.nodes[10]);
    LinkSotercNodes(&garden.nodes[5], &garden.nodes[11], &garden.nodes[12]);
    LinkSotercNodes(&garden.nodes[6], &garden.nodes[13], &garden.nodes[14]);
    LinkSotercNodes(&garden.nodes[7], &garden.nodes[15], &garden.nodes[16]);
    LinkSotercNodes(&garden.nodes[8], &garden.nodes[15], &garden.nodes[16]);
    LinkSotercNodes(&garden.nodes[9], &garden.nodes[15], &garden.nodes[16]);
    LinkSotercNodes(&garden.nodes[10], &garden.nodes[15], &garden.nodes[16]);
    LinkSotercNodes(&garden.nodes[11], &garden.nodes[17], &garden.nodes[18]);
    LinkSotercNodes(&garden.nodes[12], &garden.nodes[17], &garden.nodes[18]);
    LinkSotercNodes(&garden.nodes[13], &garden.nodes[17], &garden.nodes[18]);
    LinkSotercNodes(&garden.nodes[14], &garden.nodes[17], &garden.nodes[18]);
    LinkSotercNodes(&garden.nodes[15], &garden.nodes[19], &garden.nodes[20]);
    LinkSotercNodes(&garden.nodes[16], &garden.nodes[19], &garden.nodes[20]);
    LinkSotercNodes(&garden.nodes[17], &garden.nodes[19], &garden.nodes[20]);
    LinkSotercNodes(&garden.nodes[18], &garden.nodes[19], &garden.nodes[20]);
    LinkSotercNodes(&garden.nodes[19], &garden.nodes[21], &garden.nodes[21]);
    LinkSotercNodes(&garden.nodes[20], &garden.nodes[21], &garden.nodes[21]);
    garden.nodes[21].childLeft = NULL;
    garden.nodes[21].childRight = NULL;

    // Find initial sha512 hash of the variable length data
    uint512 hash;
    static unsigned char empty[1];
    sph_sha512_init(&garden.context_sha2);
    sph_sha512(&garden.context_sha2, (begin == end ? empty : static_cast<const void*>(&begin[0])), (end - begin) * sizeof(begin[0]));
    sph_sha512_close(&garden.context_sha2, static_cast<void*>(&hash));

#ifdef SOTERC_DEBUG
    printf("** Initial hash:\t\t%s\n", hash.ToString().c_str());
    fflush(0);
#endif

    // Assign algos to net nodes based on initial hash
    for (int i = 0; i < 22; i++)
        garden.nodes[i].algo = hash.ByteAt(i) % SOTERC_ALGO_COUNT;

    // Hardened garden gates on soterc
    if (sotercHardened)
        garden.nodes[21].algo = SOTERC_ALGO_COUNT;

    // Send the initial hash through the garden
    hash = TraverseSotercGarden(&garden, hash, &garden.nodes[0], local);

#ifdef SOTERC_DEBUG
    printf("** Soterc Final hash:\t\t\t%s\n", hash.trim256().ToString().c_str());
    fflush(0);
#endif

    // Return truncated result (take first 32 bytes of 64-byte hash)
    return hash.trim256();
}

#endif // SOTERIA_ALGO_SOTERC_H
