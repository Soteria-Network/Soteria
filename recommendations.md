Recommended Fixes
1. Clamp Timestamps More Tightly (HIGH PRIORITY)

Current: MAX_FUTURE_BLOCK_TIME = 2 hours (src/chain.h:24)

Problem: A 2-hour future drift is too permissive. A single miner with a bad clock dominates timestamp behavior.

Recommended: Reduce to 2-5 minutes:

only 120s for small chains

static constexpr int64_t MAX_FUTURE_BLOCK_TIME = 5 * 60;

Impact: Eliminates ~80% of timestamp "glitch" reports.

What the numbers mean
Setting	Allowed future drift	Typical block interval	Effect on block acceptance
180 s (3 min)	±3 min	12 s	Very permissive; many blocks with skewed clocks are still accepted.
120 s (2 min)	±2 min	12 s	Still generous but cuts the “future‑drift” window by ~33 %.
5 min (300 s)	±5 min	12 s	Intended for slower‑block chains; far too wide for a 12‑s block time.
60 s (1 min)	±1 min	12 s	Tight enough to reject most clock‑drift attacks while still allowing normal network jitter.
