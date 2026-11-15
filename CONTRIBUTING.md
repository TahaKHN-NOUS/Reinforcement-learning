# Contributing

Keep the core in simple C: small structs, direct functions, arrays, and loops.
Avoid adding frameworks or general-purpose abstractions for a single example.

Before submitting a change, run `make test`, `make sanitize`, and `make smoke`.
Changes to learning or accounting also need a fresh `make reproduce` and
`python3 tools/report.py --check-only`. Update documentation for changed
reward definitions, data boundaries, or cost assumptions. Preserve original
inputs and attribution. Do not tune on the test period and then reuse it as
an independent test. Report all predeclared seeds, including failures.
