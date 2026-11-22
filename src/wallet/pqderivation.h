// Copyright (c) 2019-2026 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_WALLET_PQDERIVATION_H
#define SOTERIA_WALLET_PQDERIVATION_H

#include "support/allocators/secure.h"
#include "uint256.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace pqderivation {

static constexpr uint32_t PURPOSE = 25;
static constexpr uint32_t ACCOUNT = 0;
static constexpr uint32_t RECEIVE_BRANCH = 0;
static constexpr uint32_t HARDENED_LIMIT = 0x80000000U;
static constexpr size_t LEGACY_SEED_BYTES = 32;
static constexpr size_t BIP39_SEED_BYTES = 64;
static constexpr size_t PQ_SEED_BYTES = 32;
static constexpr uint8_t SEED_SOURCE_LEGACY_HD = 1;
static constexpr uint8_t SEED_SOURCE_BIP39 = 2;

/**
 * Derive the ML-DSA-44 seed for m/25'/coin_type'/0'/0'/index'.
 *
 * The BIP32 leaf is the canonical 32-byte, big-endian private scalar. The
 * returned seed is SHA256(ASCII("SOTER/ML-DSA-44/keygen/v1") || leaf), with no
 * terminating NUL in the hash input.
 */
bool DeriveSeed(const unsigned char* walletSeed, size_t walletSeedLen,
                uint32_t coinType, uint32_t index, SecureVector& pqSeedOut);

/**
 * Identify one exact deterministic PQ derivation lineage.
 *
 * The digest is SHA256(ASCII("SOTER/ML-DSA-44/lineage/v1") || source ||
 * BE32(coin_type) || wallet_seed), with no terminating NUL in the hash input.
 */
bool GetLineageId(const unsigned char* walletSeed, size_t walletSeedLen,
                  uint8_t seedSource, uint32_t coinType,
                  uint256& lineageIdOut);

/** Return the human-readable path for a valid derivation index. */
std::string GetKeypath(uint32_t coinType, uint32_t index);

} // namespace pqderivation

#endif // SOTERIA_WALLET_PQDERIVATION_H
