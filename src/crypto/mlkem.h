// Copyright (c) 2025-2026 The Soteria Core developer

// ML-DSA-44 provides signatures (CHECKSIG); ML-KEM-768 provides the key
// agreement for Soteria's post-quantum P2P transport (the analog of BIP324's
// secp256k1 ECDH, which Soteria cannot use for post-quantum security).


#ifndef SOTERIA_CRYPTO_MLKEM_H
#define SOTERIA_CRYPTO_MLKEM_H

#include <cstdint>
#include <span>

namespace mlkem {

//! ML-KEM-768 byte sizes (FIPS 203).
inline constexpr size_t PUBLICKEY_SIZE = 1184;
inline constexpr size_t SECRETKEY_SIZE = 2400;
inline constexpr size_t CIPHERTEXT_SIZE = 1088;
inline constexpr size_t SHARED_SECRET_SIZE = 32;

//! Generate a keypair. Returns false if spans are mis-sized or keygen fails.
[[nodiscard]] bool Keypair(std::span<uint8_t> pk, std::span<uint8_t> sk);

//! Encapsulate to the public key `pk`: produce ciphertext `ct` and shared secret `ss`.
[[nodiscard]] bool Encapsulate(std::span<uint8_t> ct, std::span<uint8_t> ss, std::span<const uint8_t> pk);

//! Decapsulate `ct` with secret key `sk` into the shared secret `ss`.
//! ML-KEM is IND-CCA2 with implicit rejection: a tampered ciphertext yields
//! a pseudo-random (different) shared secret rather than an error.
[[nodiscard]] bool Decapsulate(std::span<uint8_t> ss, std::span<const uint8_t> ct, std::span<const uint8_t> sk);

} // namespace mlkem

#endif // SOTERIA_CRYPTO_MLKEM_H
