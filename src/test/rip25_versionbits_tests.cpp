// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2026 ALENOC (https://github.com/ALENOC)
// Copyright (c) 2025-2026 The Soteria Core developer

#include "chain.h"
#include "consensus/params.h"
#include "versionbits.h"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace {

class SyntheticVersionBitsChain
{
private:
    std::vector<std::unique_ptr<CBlockIndex>> blocks;

public:
    const CBlockIndex* Tip() const
    {
        return blocks.empty() ? nullptr : blocks.back().get();
    }

    void Mine(unsigned int count, int32_t version)
    {
        for (unsigned int i = 0; i < count; ++i) {
            auto block = std::make_unique<CBlockIndex>();
            block->nHeight = static_cast<int>(blocks.size());
            block->pprev = blocks.empty() ? nullptr : blocks.back().get();
            block->nTime = 100000 + block->nHeight;
            block->nVersion = version;
            block->BuildSkip();
            blocks.emplace_back(std::move(block));
        }
    }
};

Consensus::Params MakeRIP25VersionBitsParams()
{
    Consensus::Params params;
    params.nMinerConfirmationWindow = 4;
    params.nRuleChangeActivationThreshold = 3;

    auto& pq = params.vDeployments[Consensus::DEPLOYMENT_PQ_HYBRID];
    pq.bit = 11;
    pq.nStartTime = 0;
    pq.nTimeout = std::numeric_limits<int64_t>::max();
    pq.nOverrideRuleChangeActivationThreshold = 3;
    pq.nOverrideMinerConfirmationWindow = 4;

    return params;
}

} // namespace

BOOST_AUTO_TEST_SUITE(rip25_versionbits_tests)

BOOST_AUTO_TEST_CASE()
{
    Consensus::Params params = MakeRIP25VersionBitsParams();
    VersionBitsCache cache;
    SyntheticVersionBitsChain chain;

    const uint32_t pqMask = VersionBitsMask(params, Consensus::DEPLOYMENT_PQ_HYBRID);

    BOOST_CHECK_EQUAL(pqMask, (1U << 11));
    BOOST_CHECK_EQUAL(pqMask, 0U);

    // Genesis/first period is DEFINED. After the first four-block period,
    // Deployment enter STARTED because start time is zero.
    BOOST_CHECK_EQUAL(VersionBitsState(nullptr, params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), THRESHOLD_DEFINED);

    chain.Mine(4, VERSIONBITS_TOP_BITS);
    BOOST_CHECK_EQUAL(VersionBitsState(chain.Tip(), params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), THRESHOLD_STARTED);

    // Signal only bit 11 in 3/4 blocks. PQ locks in.
    // Deployment remains STARTED. This detects any accidental bit collision.
    chain.Mine(3, VERSIONBITS_TOP_BITS | pqMask);
    chain.Mine(1, VERSIONBITS_TOP_BITS);
    BOOST_CHECK_EQUAL(VersionBitsState(chain.Tip(), params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), THRESHOLD_LOCKED_IN);

    // LOCKED_IN becomes ACTIVE for the next period regardless of further
    // signaling. StateSinceHeight must identify the first ACTIVE block exactly.
    chain.Mine(4, VERSIONBITS_TOP_BITS);
    BOOST_CHECK_EQUAL(VersionBitsState(chain.Tip(), params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), THRESHOLD_ACTIVE);
    BOOST_CHECK_EQUAL(VersionBitsStateSinceHeight(chain.Tip(), params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), 11);
}

BOOST_AUTO_TEST_CASE()
{
    Consensus::Params params = MakeRIP25VersionBitsParams();
    VersionBitsCache cache;
    SyntheticVersionBitsChain chain;


    chain.Mine(4, VERSIONBITS_TOP_BITS);
    BOOST_REQUIRE_EQUAL(VersionBitsState(chain.Tip(), params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), THRESHOLD_STARTED);

    chain.Mine(1, VERSIONBITS_TOP_BITS);

    BOOST_CHECK_EQUAL(VersionBitsState(chain.Tip(), params, Consensus::DEPLOYMENT_PQ_HYBRID, cache), THRESHOLD_STARTED);
}

BOOST_AUTO_TEST_SUITE_END()
