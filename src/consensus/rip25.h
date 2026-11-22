// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_CONSENSUS_RIP25_H
#define SOTERIA_CONSENSUS_RIP25_H

#include <array>
#include <cstddef>

namespace Consensus {

constexpr std::size_t PQ_SIGNATURE_CONTEXT_BYTES = 83; // Soter will use 83
using PQSignatureContext = std::array<unsigned char, PQ_SIGNATURE_CONTEXT_BYTES>;

inline bool IsValidPQSignatureContext(const PQSignatureContext& context)
{
    // Canonical form is lowercase hex. Uppercase A–F is rejected by
    // design; producers must lowercase-normalize before constructing
    // the context. Do not relax this without a canonicalization step
    // at the RPC/wallet boundary — otherwise nodes may disagree on
    // byte-identical-but-differently-cased contexts.
    static const unsigned char prefix[] = "SOTER/ML-DSA-44/v1/"; // 19 char
    static_assert(sizeof(prefix) - 1 == 19, "unexpected RIP-25 context prefix length"); 
    static_assert(sizeof(prefix) - 1 + 64 == PQ_SIGNATURE_CONTEXT_BYTES,
              "context size mismatch");
    for (std::size_t i = 0; i < sizeof(prefix) - 1; ++i) {
        if (context[i] != prefix[i])
            return false;
    }
    for (std::size_t i = sizeof(prefix) - 1; i < context.size(); ++i) {
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

} 

#endif 
