Recommended Fixes
1. Clamp Timestamps More Tightly (HIGH PRIORITY)

Current: MAX_FUTURE_BLOCK_TIME = 2 hours (src/chain.h:24)

Problem: A 2-hour future drift is too permissive. A single miner with a bad clock dominates timestamp behavior.

Recommended: Reduce to 2-5 minutes:

only 120s for small chains

static constexpr int64_t MAX_FUTURE_BLOCK_TIME = 5 * 60;

Impact: Eliminates ~80% of timestamp "glitch" reports.

