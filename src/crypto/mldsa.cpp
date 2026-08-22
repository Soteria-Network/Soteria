// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2026 ALENOC (https://github.com/ALENOC)
// Copyright (c) 2025-2026 The Soteria Core developer
// RIP-25: ML-DSA-44 (FIPS 204) Post-Quantum Digital Signature Implementation
// Uses liboqs (Open Quantum Safe) for NIST FIPS 204 compliant ML-DSA-44.
// https://github.com/open-quantum-safe/liboqs

#include "mldsa.h"

#include <oqs/oqs.h>

#include <array>
#include <cstring>
#include <mutex>

// Compile-time checks: ensure our constants match liboqs.
static_assert(mldsa::PUBLICKEY_BYTES == OQS_SIG_ml_dsa_44_length_public_key,
              "ML-DSA-44 public key size mismatch with liboqs");
static_assert(mldsa::SECRETKEY_BYTES == OQS_SIG_ml_dsa_44_length_secret_key,
              "ML-DSA-44 secret key size mismatch with liboqs");
static_assert(mldsa::SIGNATURE_BYTES == OQS_SIG_ml_dsa_44_length_signature,
              "ML-DSA-44 signature size mismatch with liboqs");

namespace {

// liboqs exposes a process-wide randombytes provider. ML-DSA-44 in liboqs
// 0.12.0 obtains exactly SEED_BYTES of entropy when creating a keypair. We
// serialize every wrapper operation that can consume OQS randomness so the
// temporary deterministic provider can never leak into another Soter ML-DSA
// operation.
std::mutex g_oqs_rng_mutex;
std::array<unsigned char, mldsa::SEED_BYTES> g_deterministic_seed{};
size_t g_deterministic_offset = 0;
bool g_deterministic_rng_error = false;

void DeterministicRandomBytes(uint8_t* out, size_t bytes_to_read)
{
    if (!out || g_deterministic_offset > mldsa::SEED_BYTES ||
        bytes_to_read > mldsa::SEED_BYTES - g_deterministic_offset) {
        if (out && bytes_to_read)
            std::memset(out, 0, bytes_to_read);
        g_deterministic_rng_error = true;
        return;
    }

    std::memcpy(out, g_deterministic_seed.data() + g_deterministic_offset, bytes_to_read);
    g_deterministic_offset += bytes_to_read;
}

bool RestoreSystemRng()
{
    const bool restored = OQS_randombytes_switch_algorithm(OQS_RAND_alg_system) == OQS_SUCCESS;
    g_deterministic_seed.fill(0);
    g_deterministic_offset = 0;
    g_deterministic_rng_error = false;
    return restored;
}

} // namespace

namespace mldsa {

bool KeyGen(unsigned char* pk, unsigned char* sk, const unsigned char* seed)
{
    if (!pk || !sk || !seed)
        return false;

    std::lock_guard<std::mutex> lock(g_oqs_rng_mutex);

    std::memcpy(g_deterministic_seed.data(), seed, SEED_BYTES);
    g_deterministic_offset = 0;
    g_deterministic_rng_error = false;
    OQS_randombytes_custom_algorithm(DeterministicRandomBytes);

    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_44);
    if (!sig) {
        RestoreSystemRng();
        return false;
    }

    const OQS_STATUS rc = OQS_SIG_keypair(sig, pk, sk);
    OQS_SIG_free(sig);

    const bool consumed_expected_seed = !g_deterministic_rng_error &&
                                        g_deterministic_offset == SEED_BYTES;
    const bool restored = RestoreSystemRng();
    return rc == OQS_SUCCESS && consumed_expected_seed && restored;
}

bool KeyGenRandom(unsigned char* pk, unsigned char* sk)
{
    if (!pk || !sk)
        return false;

    std::lock_guard<std::mutex> lock(g_oqs_rng_mutex);

    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_44);
    if (!sig)
        return false;

    const OQS_STATUS rc = OQS_SIG_keypair(sig, pk, sk);
    OQS_SIG_free(sig);

    return rc == OQS_SUCCESS;
}

bool Sign(unsigned char* sig, size_t* siglen,
          const unsigned char* msg, size_t msglen,
          const unsigned char* sk)
{
    if (!sig || !siglen || !msg || !sk)
        return false;

    // liboqs 0.12.0 signs deterministically by default, but keep signing under
    // the RNG mutex so builds that enable randomized ML-DSA signing cannot race
    // a deterministic KeyGen() provider switch.
    std::lock_guard<std::mutex> lock(g_oqs_rng_mutex);

    OQS_SIG* signer = OQS_SIG_new(OQS_SIG_alg_ml_dsa_44);
    if (!signer)
        return false;

    const OQS_STATUS rc = OQS_SIG_sign(signer, sig, siglen, msg, msglen, sk);
    OQS_SIG_free(signer);

    return rc == OQS_SUCCESS;
}

bool Verify(const unsigned char* sig, size_t siglen,
            const unsigned char* msg, size_t msglen,
            const unsigned char* pk)
{
    if (!sig || !msg || !pk)
        return false;

    if (siglen != SIGNATURE_BYTES)
        return false;

    OQS_SIG* verifier = OQS_SIG_new(OQS_SIG_alg_ml_dsa_44);
    if (!verifier)
        return false;

    const OQS_STATUS rc = OQS_SIG_verify(verifier, msg, msglen, sig, siglen, pk);
    OQS_SIG_free(verifier);

    return rc == OQS_SUCCESS;
}

} // namespace mldsa
