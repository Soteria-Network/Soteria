// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_CONSENSUS_RIP25_H
#define SOTERIA_CONSENSUS_RIP25_H

#include <array>
#include <cstddef>

namespace Consensus {

// RIP-25 ML-DSA-44 signing context wire format (consensus-critical, 83 bytes):
//   "SOTER/ML-DSA-44/v1/"  (19-byte ASCII domain-separation prefix)
//   + 64 lowercase hex chars  (32-byte digest, ASCII-encoded)
//   = 83 bytes total.
constexpr std::size_t PQ_SIGNATURE_CONTEXT_BYTES = 83;

inline constexpr char PQ_SIGNATURE_CONTEXT_PREFIX[] = "SOTER/ML-DSA-44/v1/";
constexpr std::size_t PQ_SIGNATURE_CONTEXT_PREFIX_BYTES =
    sizeof(PQ_SIGNATURE_CONTEXT_PREFIX) - 1;   // drop trailing NUL
constexpr std::size_t PQ_SIGNATURE_HEX_BYTES = 64;

// Invariants: fire on every TU, not only when the validator is used.
static_assert(PQ_SIGNATURE_CONTEXT_PREFIX_BYTES == 19,
              "unexpected RIP-25 prefix length");
static_assert(PQ_SIGNATURE_CONTEXT_PREFIX_BYTES + PQ_SIGNATURE_HEX_BYTES ==
                  PQ_SIGNATURE_CONTEXT_BYTES,
              "prefix + hex must equal the 83-byte RIP-25 context");

using PQSignatureContext = std::array<unsigned char, PQ_SIGNATURE_CONTEXT_BYTES>;

/**
 * Validate an RIP-25 ML-DSA-44 signing context.
 *
 * Canonical form is lowercase hex: the 64 characters after the
 * "SOTER/ML-DSA-44/v1/" prefix MUST be in [0-9a-f]. Uppercase A-F is
 * rejected by design; producers (wallets, RPC, test vectors) must
 * lowercase-normalize the digest before constructing the context.
 * Do not relax this without a canonicalization step at the RPC/wallet
 * boundary -- otherwise nodes may disagree on byte-identical-but-
 * differently-cased contexts.
 *
 * constexpr so callers can static_assert it.
 */
constexpr bool IsValidPQSignatureContext(const PQSignatureContext& context)
{
    for (std::size_t i = 0; i < PQ_SIGNATURE_CONTEXT_PREFIX_BYTES; ++i) {
        if (context[i] != static_cast<unsigned char>(PQ_SIGNATURE_CONTEXT_PREFIX[i]))
            return false;
    }
    for (std::size_t i = PQ_SIGNATURE_CONTEXT_PREFIX_BYTES;
         i < PQ_SIGNATURE_CONTEXT_BYTES; ++i) {
        const unsigned char ch = context[i];
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
            return false;
    }
    return true;
}

inline const PQSignatureContext& NullPQSignatureContext()
{
    static const PQSignatureContext context{};
    return context;
}

} // namespace Consensus

#endif // SOTERIA_CONSENSUS_RIP25_H
