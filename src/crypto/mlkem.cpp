// Copyright (c) 2025-2026 The Soteria Core developer

#include <crypto/mlkem.h>
#include <oqs/oqs.h>

namespace mlkem {

namespace {
// Helper to create the liboqs ML-KEM-768 object.
OQS_KEM* NewKem()
{
    return OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
}
} // namespace

bool Keypair(std::span<uint8_t> pk, std::span<uint8_t> sk)
{
    if (pk.size() != PUBLICKEY_SIZE || sk.size() != SECRETKEY_SIZE) return false;

    OQS_KEM* kem = NewKem();
    if (!kem) return false;

    bool ok = OQS_KEM_keypair(kem, pk.data(), sk.data()) == OQS_SUCCESS;
    OQS_KEM_free(kem);
    return ok;
}

bool Encapsulate(std::span<uint8_t> ct, std::span<uint8_t> ss, std::span<const uint8_t> pk)
{
    if (ct.size() != CIPHERTEXT_SIZE || ss.size() != SHARED_SECRET_SIZE || pk.size() != PUBLICKEY_SIZE) return false;

    OQS_KEM* kem = NewKem();
    if (!kem) return false;

    bool ok = OQS_KEM_encaps(kem, ct.data(), ss.data(), pk.data()) == OQS_SUCCESS;
    OQS_KEM_free(kem);
    return ok;
}

bool Decapsulate(std::span<uint8_t> ss, std::span<const uint8_t> ct, std::span<const uint8_t> sk)
{
    if (ss.size() != SHARED_SECRET_SIZE || ct.size() != CIPHERTEXT_SIZE || sk.size() != SECRETKEY_SIZE) return false;

    OQS_KEM* kem = NewKem();
    if (!kem) return false;

    bool ok = OQS_KEM_decaps(kem, ss.data(), ct.data(), sk.data()) == OQS_SUCCESS;
    OQS_KEM_free(kem);
    return ok;
}

} // namespace mlkem
