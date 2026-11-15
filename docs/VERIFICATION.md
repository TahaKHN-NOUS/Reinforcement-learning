# Executed verification

Verified on 2026-11-15 with GCC 14.2.0, C11, strict warnings treated as errors:

- 35 C unit/integration checks passed.
- The same 35 checks passed under AddressSanitizer, UndefinedBehaviorSanitizer,
  and LeakSanitizer, with no reported errors.
- Sanitized command-line maze and real-market examples completed cleanly.
- Both learning methods reached the BFS shortest path in all ten seeds on
  both fixed mazes: 45 and 10 moves respectively.
- Both market experiments completed all ten seeds per learned method and
  all five fixed-policy transaction-cost scenarios.
- The independent Python audit reconciled training-only feature thresholds,
  validation selection, saved-policy actions, twelve example ledgers, fees,
  compounded wealth, drawdown, Sharpe, and cost sensitivity.
- The 22 original text/source inputs match their uploaded archive bytes.
- All input/source checksums and generated SVG documents verified.

Evidence is recorded in `results/unit_tests.log`, `sanitizers.log`,
`sanitizer_examples.log`, `reproduction.log`, and `independent_audit.log`.
The workflow under `.github/workflows/` provides remote checks; a configured
workflow by itself is not a claim that GitHub has run it successfully.

The delivered real example selects cash using validation. Mean test returns
are approximately +0.05% for Q-learning, −0.81% for Double Q-learning, and
+1.21% for buy-and-hold, after 1 bp per-side assumed costs. No claim of an RL
trading edge is made. See `RESULTS.md` for all seed ranges and baselines.
