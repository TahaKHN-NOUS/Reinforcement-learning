# Data and provenance

## Real example: EUR/USD

`eurusd_daily.csv` is derived from the project owner's previously supplied
`EURUSD-2000-2020-15m.csv` (500,751 OHLC rows). The source file is available in
the owner's [Projet_STA2 repository](https://github.com/TahaKHN-NOUS/Projet_STA2/blob/main/data/raw/EURUSD-2000-2020-15m.csv).

Original source SHA256:

```text
d70000039375de84703c2c5e41dd0d5039bb60106bf05f036ed401233b86ea12
```

The included derived file contains 5,254 observed source-clock dates from
2000-01-03 through 2020-03-18. For each date, OPEN is the first available
bar's open and CLOSE is the last available bar's close. The final source date,
2020-03-19, is excluded because the file ends at 06:00 and daily completion
is not established. A day is emitted when a later source date is observed.

All other observed dates are kept, including 661 dates with fewer than 96
bars. No future intraday completeness rule controls whether the agent trades
at a day's open. No OHLC values, missing days, or missing intraday bars are
invented. A previous day's close can be stale if the source omitted its last
bars. Open-to-next-open returns span any intervening calendar gap; features
count observed sessions, not necessarily consecutive calendar days.

The final retained row provides the test's terminal liquidation open; its
close is not used for a test action. All price history preceding a validation
or test start can supply past-only feature warmup.

To rebuild from the supplied raw input:

```sh
./rl prepare /path/to/EURUSD-2000-2020-15m.csv data/eurusd_daily.csv
```

The vendor, timezone, original quote type, and market-data license were not
provided. Retaining the supplied data does not grant a new upstream license.
The conversion is an observed-price educational dataset, not a reconstructed
exchange calendar or broker execution feed.

## Synthetic example

`synthetic_prices.csv` contains 3,000 artificial weekday sessions generated
with fixed seed 12345. It has weak positive drift, modest return persistence,
alternating volatility, and small open gaps. It is explicitly synthetic,
not CAC40 data or evidence of market profitability:

```sh
./rl demo data/synthetic_prices.csv
```

## Original toy data

`originals/data/train.txt` contains 18 prices and `real.txt` one comparison
price. No dates, year, vendor, or sufficiently long evaluation sample were
supplied. They are preserved unchanged; the new trading study does not
pretend to reproduce an original learned CAC40 price forecast.

## Mazes

`maze.txt` preserves the original 12×20 layout. `maze_small.txt` is a second
7×9 benchmark. Symbols are `+` for wall, space for traversable cell, `s` for
start, and `g` for goal. Border walls are optional because movement checks
bounds before accessing a cell. Maximum dimensions are 40×40.
