# What changed from the original project

The supplied archive is recorded in `ORIGINAL_ARCHIVE.sha256`. Original source,
headers, README, Makefile, and input data remain byte-for-byte under `originals/`.
Bundled compiled `.x`/`.exe` files are not copied or executed.

| Original issue | Rebuild |
|---|---|
| Bounds accessed before movement was clamped | Validate coordinates before touching the maze |
| New visited allocations leaked every episode | Fixed-size, local maze state; no per-episode allocation |
| DFS markings remained on RL displays | Immutable grid and separate learned-path rendering |
| Terminal updates bootstrapped future Q values | Explicit terminal target equals immediate reward |
| Very low fixed exploration; one sequential run | Decaying exploration and ten independently reset seeds |
| No frozen maze evaluation | Greedy no-update evaluation against BFS/DFS |
| Double Q ambiguously documented as trading code | Same Double Q implementation supports both examples |
| Trading state was `(day, position)` | Recurring return/momentum/volatility/exposure state |
| Sale reward depended on omitted purchase price | Mark-to-market wealth reward depends on exposure and return |
| Sale used tomorrow's price without a consistent convention | Prior-close signal, open-to-next-open holding interval |
| Only 18 training prices and one comparison value | Included real daily EUR/USD and seeded synthetic datasets |
| BUY/SELL/HOLD converted to a hardcoded ±1% price guess | No fabricated forecast; explicit saved-policy action inference |
| Final flat state was all zeros, tie defaulted to BUY | State coverage recorded; unseen market states target flat |
| Episodes limited to 250 steps even on longer datasets | Training traverses every training interval |
| No chronological validation or trading benchmarks | 60/20/20 split, validation selection, cash/hold/momentum controls |
| No auditable costs or final liquidation | Turnover fees, carried exposure, terminal fees, full ledgers |
| No model persistence, tests, or CI | Text model files, training config, C tests, sanitizers, CI |

Original executable observations, before rebuilding:

- Original maze Q-learning reached 45 moves by episode 500; independent BFS
  confirmed 45 was optimal. Double Q-learning hit the 250-step cap at episode 500.
- The reported CAC40 price forecast was 7,905.06 with 1.98% relative error.
  Reusing the last price, 7,826.79, gave 0.97% error against 7,751.89.
- The queried final flat-state Q values were exactly `(0,0,0)`, so deterministic
  tie-breaking chose BUY and triggered the artificial +1% forecast.
- Identical encoded trading states gave different sale rewards depending on
  omitted entry-price history. Maze memory leaks were confirmed by sanitizer.

The rebuild replaces the flawed trading demonstration. It does not claim to
reproduce a valid original CAC40 forecasting study, and it does not claim that
Double Q-learning must outperform ordinary Q-learning.
