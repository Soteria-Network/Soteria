// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2026 ALENOC (https://github.com/ALENOC)
// Copyright (c) 2025-2026 The Soteria Core developer

// RIP-25: ML-DSA-44 (FIPS 204) Post-Quantum Digital Signature Wrapper
// Uses liboqs (Open Quantum Safe) for the underlying implementation.

#ifndef SOTERIA_CRYPTO_MLDSA_H
#define SOTERIA_CRYPTO_MLDSA_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mldsa {

// ML-DSA-44 (FIPS 204) constants — must match OQS_SIG_ml_dsa_44 values
static const size_t PUBLICKEY_BYTES  = 1312;
static const size_t SECRETKEY_BYTES  = 2560;
static const size_t SIGNATURE_BYTES  = 2420;
static const size_t SEED_BYTES       = 32;

/**
 * Generate an ML-DSA-44 keypair from a 32-byte seed.
 * Deterministic: same seed always produces the same keypair.
 * liboqs 0.12.0 has no public seeded-signature keypair API, so the wrapper
 * temporarily supplies the seed through liboqs' public custom-randombytes API
 * while serializing all Soter operations that can consume OQS randomness.
 *
 * @param[out] pk   Public key buffer (must be PUBLICKEY_BYTES)
 * @param[out] sk   Secret key buffer (must be SECRETKEY_BYTES)
 * @param[in]  seed 32-byte seed
 * @return true on success
 */
bool KeyGen(unsigned char* pk, unsigned char* sk, const unsigned char* seed);

/**
 * Generate an ML-DSA-44 keypair from random entropy.
 * Uses OQS_SIG_keypair() internally.
 *
 * @param[out] pk  Public key buffer (must be PUBLICKEY_BYTES)
 * @param[out] sk  Secret key buffer (must be SECRETKEY_BYTES)
 * @return true on success
 */
bool KeyGenRandom(unsigned char* pk, unsigned char* sk);

/**
 * Sign a message using ML-DSA-44.
 * Uses OQS_SIG_sign() internally.
 *
 * @param[out] sig     Signature buffer (must be SIGNATURE_BYTES)
 * @param[out] siglen  Actual signature length (always SIGNATURE_BYTES for ML-DSA-44)
 * @param[in]  msg     Message to sign
 * @param[in]  msglen  Message length
 * @param[in]  sk      Secret key (SECRETKEY_BYTES)
 * @return true on success
 */
bool Sign(unsigned char* sig, size_t* siglen,
          const unsigned char* msg, size_t msglen,
          const unsigned char* sk);

/**
 * Verify an ML-DSA-44 signature.
 * Uses OQS_SIG_verify() internally.
 *
 * @param[in] sig     Signature (SIGNATURE_BYTES)
 * @param[in] siglen  Signature length
 * @param[in] msg     Message
 * @param[in] msglen  Message length
 * @param[in] pk      Public key (PUBLICKEY_BYTES)
 * @return true if signature is valid
 */
bool Verify(const unsigned char* sig, size_t siglen,
            const unsigned char* msg, size_t msglen,
            const unsigned char* pk);

} // namespace mldsa

#endif 
