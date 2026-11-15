# Reproduction and verification

## Core requirements

A C11 compiler, `make`, and the standard C/math libraries. There are no downloaded
C dependencies. The release was executed with GCC 14.2.0 on Linux. The GitHub
workflow checks Linux and macOS; inspect the remote workflow for its outcome.
Python 3 is optional and only used for an independent audit and reporting.
Its script uses the standard library; no pip packages are required.

From the project root:

```sh
make all test
make sanitize
make reproduce
python3 tools/report.py
./rl signal data/eurusd_daily.csv results/real 0
```

The full reproduction uses all ten predeclared seeds per learning algorithm,
two mazes, and both market datasets. A short smoke run uses one seed and a
synthetic market. Reproduction overwrites the corresponding CSVs and text
policies; it never deletes input data.

To rebuild real daily data from the prior project's supplied raw file:

```sh
./rl prepare ../Projet_STA2/data/raw/EURUSD-2000-2020-15m.csv data/eurusd_daily.csv
```

The prior project's directory location can differ; supply the real path.
The included derived data is sufficient to run every experiment offline.
Use `sha256sum -c docs/DATA_CHECKSUMS.sha256` to verify it. Source checksums
record the code of this delivered version. Small compiler/floating-point
changes can affect rare ties; exact numerical identity across toolchains is
not promised. Seed schedules and data are fixed, never chosen for test profit.

## Result directories

`results/maze/` and `results/maze_small/` contain scores for all seeds,
representative learning curves, and actual learned paths for seed 11.

`results/real/` and `results/synthetic/` each contain:

- `trading_scores.csv`: validation scores and all test cost/seed combinations.
- `trading_config.txt`: data path, split boundaries, parameters, training-only
  thresholds, validation scores, selection, and representative seed.
- `trading_*_policy.txt`: text Q tables and state visit counts for seed 11.
- `trading_state_visits.csv`: training coverage for those two example agents.
- `trading_*_curve.csv`: exploratory training returns, not held-out performance.
- `trading_*_ledger.csv`: per-interval actions, fees, returns, wealth, and dates
  at 1 bp per side for each example policy and the validation-selected policy.
- `example_signal.txt`: a historical action from reloading the saved selection.
- `returns.svg`: all-seed return comparison created by the optional report.

Different training seeds produce different learned policies. Test ledgers
show only the first fixed seed for each learned method; score tables report
all seeds. Validation chooses a method using average log wealth, not a winning
seed or the final test result.

## Checks

C tests exercise terminal Q and Double Q targets, cross-table evaluation,
random tie-breaking, policy persistence, maze boundaries, absorbing goals,
shortest paths, map immutability, training-only cuts, past-only features,
state coverage, full-length episodes, buy/hold/sell costs, terminal liquidation,
cash, direct buy-and-hold reconciliation, frozen tables, invalid inputs, and
aggregation of incomplete observed days without inventing bars.

`make sanitize` reruns those tests with address, undefined-behavior, and leak
checks. Linux/macOS CI builds with warnings treated as errors; Linux runs the
sanitizers. The optional Python audit independently reconstructs training
thresholds, validation selection, saved-policy actions, all six ledgers in
each market example, fees, wealth, Sharpe, and drawdown from the CSV inputs.

These checks establish implementation properties and saved-result consistency.
They do not validate market-data provenance, realistic fills, financing, or
an investment edge. See `METHOD.md` for financial and statistical limits.
