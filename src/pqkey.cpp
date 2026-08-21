// Copyright (c) 2017-2020 The Raven Core developers
// Copyright (c) 2026 ALENOC (https://github.com/ALENOC)
// Copyright (c) 2025-2026 The Soteria Core developer

// RIP-25: ML-DSA-44 Post-Quantum Key Implementation

#include "pqkey.h"
#include "crypto/sha256.h"

#include <cstring>

// For random keygen
extern void GetStrongRandBytes(unsigned char* buf, int num);

// --- CPQPubKey ---

uint256 CPQPubKey::GetWitnessProgram() const
{
    uint256 result;
    CSHA256 hasher;
    hasher.Write(vch.data(), vch.size());
    hasher.Finalize(result.begin());
    return result;
}

bool CPQPubKey::Verify(const uint256& hash, const std::vector<unsigned char>& sig) const
{
    if (!IsValid())
        return false;

    if (sig.size() != mldsa::SIGNATURE_BYTES)
        return false;

    return mldsa::Verify(sig.data(), sig.size(),
                         hash.begin(), 32,
                         vch.data());
}

// --- CPQKey ---

void CPQKey::MakeNewKey()
{
    unsigned char pk[mldsa::PUBLICKEY_BYTES];

    if (!mldsa::KeyGenRandom(pk, keydata.data())) {
        fValid = false;
        pubkey = CPQPubKey();
        return;
    }

    pubkey = CPQPubKey(pk, pk + mldsa::PUBLICKEY_BYTES);
    fValid = true;
}

bool CPQKey::SetSeed(const unsigned char* seed)
{
    if (!seed)
        return false;

    unsigned char pk[mldsa::PUBLICKEY_BYTES];

    if (!mldsa::KeyGen(pk, keydata.data(), seed)) {
        fValid = false;
        pubkey = CPQPubKey();
        return false;
    }

    pubkey = CPQPubKey(pk, pk + mldsa::PUBLICKEY_BYTES);
    fValid = true;
    return true;
}

bool CPQKey::Sign(const uint256& hash, std::vector<unsigned char>& sigOut) const
{
    if (!fValid)
        return false;

    sigOut.resize(mldsa::SIGNATURE_BYTES);
    size_t siglen = 0;

    if (!mldsa::Sign(sigOut.data(), &siglen,
                     hash.begin(), 32,
                     keydata.data())) {
        sigOut.clear();
        return false;
    }

    if (siglen != mldsa::SIGNATURE_BYTES) {
        sigOut.clear();
        return false;
    }

    return true;
}

bool CPQKey::SetKeyData(const std::vector<unsigned char>& data)
{
    if (data.size() != mldsa::SECRETKEY_BYTES) {
        fValid = false;
        pubkey = CPQPubKey();
        return false;
    }

    std::memcpy(keydata.data(), data.data(), mldsa::SECRETKEY_BYTES);
    pubkey = CPQPubKey();
    fValid = true;
    return true;
}

bool CPQKey::MatchesPubKey(const CPQPubKey& pubkeyIn) const
{
    if (!fValid || !pubkeyIn.IsValid())
        return false;

    // Fixed non-secret challenge: possession of the secret key is proven by
    // producing a valid ML-DSA signature that verifies under pubkeyIn.
    uint256 challenge;
    std::memset(challenge.begin(), 0x53, 32);

    std::vector<unsigned char> sig;
    if (!Sign(challenge, sig))
        return false;

    return pubkeyIn.Verify(challenge, sig);
}

bool CPQKey::SetKeyData(const std::vector<unsigned char>& data, const CPQPubKey& pubkeyIn)
{
    if (!SetKeyData(data))
        return false;

    if (!MatchesPubKey(pubkeyIn)) {
        memory_cleanse(keydata.data(), keydata.size());
        pubkey = CPQPubKey();
        fValid = false;
        return false;
    }

    pubkey = pubkeyIn;
    return true;
}
