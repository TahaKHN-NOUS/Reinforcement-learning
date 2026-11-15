# Changes

## 1.0.0 — project rebuild

- Preserved and credited the original ENSTA Paris source and toy datasets.
- Shared simple Q-learning/Double Q-learning agent across maze and trading.
- Fixed memory management, bounds, terminal targets, and learned-path rendering.
- Added BFS/DFS benchmarks, two mazes, frozen evaluation, and multiple seeds.
- Replaced day-index trading states and artificial price forecasts with past-only
  market features and an exposure-based portfolio simulation.
- Added turnover costs, final liquidation, chronological validation/test splits,
  cash/hold/momentum benchmarks, and fixed-policy cost sensitivity.
- Added real and synthetic data, model persistence, historical signal inference,
  transparent results, independent accounting audit, tests, and automated CI.
