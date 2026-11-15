"""Optional: independently audit saved C results and build Markdown/SVG reports.
Python standard library only. Run from the project root.
"""
import argparse
import csv
import html
import math
import statistics
from pathlib import Path


def rows(path):
    with open(path, newline="") as file:
        return list(csv.DictReader(file))


def config(path):
    return dict(line.split("=", 1) for line in Path(path).read_text().splitlines())


def saved_agent(path):
    tokens = iter(Path(path).read_text().split())
    assert next(tokens) == "TABULAR_RL_2"
    states, actions, double = [int(next(tokens)) for _ in range(3)]
    table, visits = [], []
    for _ in range(states):
        visits.append(int(next(tokens)))
        values = []
        for _ in range(actions):
            q1, q2 = float(next(tokens)), float(next(tokens))
            values.append(q1 + (q2 if double else 0))
        table.append(values)
    return table, visits


def feature(bars, k):
    close = lambda j: float(bars[j]["close"])
    r = math.log(close(k) / close(k - 1))
    m = math.log(close(k) / close(k - 5))
    v = math.sqrt(sum(math.log(close(j) / close(j - 1)) ** 2 for j in range(k - 19, k + 1)) / 20)
    return r, m, v


def state(bars, time, position, cfg):
    r, m, v = feature(bars, time - 1)
    rbin = 0 if r < float(cfg["return_low"]) else (1 if r < float(cfg["return_high"]) else 2)
    mbin = 0 if m < float(cfg["momentum_low"]) else (1 if m < float(cfg["momentum_high"]) else 2)
    vbin = int(v >= float(cfg["volatility_mid"]))
    return ((rbin * 3 + mbin) * 2 + vbin) * 2 + position


def audit(directory):
    cfg = config(directory / "trading_config.txt")
    bars = rows(cfg["data"])
    train_end = int(cfg["train_end_exclusive"])
    validation_end = int(cfg["validation_end_exclusive"])
    assert len(bars) == int(cfg["bars"])
    features = [feature(bars, k) for k in range(20, train_end)]
    r, m, v = [sorted(x[i] for x in features) for i in range(3)]
    n = len(features)
    for key, value in zip(("return_low", "return_high", "momentum_low", "momentum_high", "volatility_mid"),
                          (r[n // 3], r[2 * n // 3], m[n // 3], m[2 * n // 3], v[n // 2])):
        assert abs(float(cfg[key]) - value) < 1e-12
    scores = rows(directory / "trading_scores.csv")
    # Evaluation costs must not change a policy's exposure sequence.
    keys = {(x["policy"], x["seed"]) for x in scores if x["phase"] == "test"}
    for policy, seed in keys:
        group = sorted((x for x in scores if x["phase"] == "test" and x["policy"] == policy
                        and x["seed"] == seed), key=lambda x: float(x["one_way_cost_bps"]))
        assert [float(x["one_way_cost_bps"]) for x in group] == [0, 0.5, 1, 2, 5]
        assert len({x["orders"] for x in group}) == 1
        assert max(float(x["gross_return"]) for x in group) - min(float(x["gross_return"]) for x in group) < 1e-12
        net = [float(x["net_return"]) for x in group]
        assert all(a >= b - 1e-12 for a, b in zip(net, net[1:]))
    mean_log = {}
    for method in ("q_learning", "double_q"):
        group = [x for x in scores if x["phase"] == "validation" and x["policy"] == method]
        assert len(group) == int(cfg["seeds"])
        mean_log[method] = statistics.mean(math.log1p(float(x["net_return"])) for x in group)
    selected = max(("cash", "q_learning", "double_q"), key=lambda p: mean_log.get(p, 0))
    assert selected == cfg["selected_policy"]
    for name in ("q_learning", "double_q", "cash", "buy_hold", "momentum", "selected"):
        policy = selected if name == "selected" else name
        ledger = rows(directory / f"trading_{name}_ledger.csv")
        assert len(ledger) == len(bars) - validation_end - 1
        table, visits = saved_agent(directory / f"trading_{policy}_policy.txt") if policy in mean_log else (None, None)
        nav, gross, peak, drawdown, fees, orders, previous = 10000.0, 1.0, 10000.0, 0.0, 0.0, 0, 0
        net_returns = []
        for j, line in enumerate(ledger):
            t = validation_end + j
            assert (line["date"], line["next_date"]) == (bars[t]["date"], bars[t + 1]["date"])
            s = state(bars, t, previous, cfg)
            assert s == int(line["state"])
            if table is not None:
                action = max(range(3), key=lambda a: table[s][a]) if visits[s] else 1
            elif policy == "cash":
                action = 1
            elif policy == "buy_hold":
                action = 0
            else:
                action = 0 if feature(bars, t - 1)[1] > 0 else 1
            assert action == int(line["action"])
            position = 1 if action == 0 else (0 if action == 1 else previous)
            assert position == int(line["position"])
            r = float(bars[t + 1]["open"]) / float(bars[t]["open"]) - 1
            trade_fee = nav * abs(position - previous) * 0.0001
            after = (nav - trade_fee) * (1 + position * r)
            terminal = j == len(ledger) - 1
            exit_fee = after * position * 0.0001 if terminal else 0
            after -= exit_fee
            realized = after / nav - 1
            assert abs(float(line["price_return"]) - r) < 1e-12
            assert abs(float(line["trade_fee"]) - trade_fee) < 1e-7
            assert abs(float(line["liquidation_fee"]) - exit_fee) < 1e-7
            assert abs(float(line["nav"]) - after) < 1e-7
            assert abs(float(line["net_return"]) - realized) < 1e-12
            gross *= 1 + position * r
            fees += trade_fee + exit_fee
            orders += abs(position - previous) + (position if terminal else 0)
            nav = after
            peak = max(peak, nav)
            drawdown = min(drawdown, nav / peak - 1)
            net_returns.append(realized)
            previous = 0 if terminal else position
        scored = [x for x in scores if x["phase"] == "test" and x["policy"] == policy
                  and float(x["one_way_cost_bps"]) == 1 and int(x["seed"]) == (11 if table else 0)]
        assert len(scored) == 1
        scored = scored[0]
        for key, actual in (("net_return", nav / 10000 - 1), ("gross_return", gross - 1),
                            ("max_drawdown", drawdown), ("fees_on_10000", fees)):
            assert abs(float(scored[key]) - actual) < 1e-7
        assert int(scored["orders"]) == orders
        sd = statistics.stdev(net_returns)
        if sd > 1e-10:
            sharpe = math.sqrt(252) * statistics.mean(net_returns) / sd
            assert abs(float(scored["session_sharpe_252"]) - sharpe) < 1e-7
    print(f"PASS: {directory}: training-only bins, validation selection, saved policies, six ledgers, fees and wealth reconciled.")
    return cfg, scores


def return_chart(scores, output, title):
    methods = ["cash", "buy_hold", "momentum", "q_learning", "double_q"]
    groups = [[100 * float(x["net_return"]) for x in scores if x["phase"] == "test"
               and x["policy"] == m and float(x["one_way_cost_bps"]) == 1] for m in methods]
    low = min(0, min(map(min, groups))) - 2
    high = max(0, max(map(max, groups))) + 2
    x = lambda v: 190 + (v - low) / (high - low) * 620
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="400" viewBox="0 0 1000 400">',
             '<rect width="1000" height="400" fill="white"/>',
             f'<text x="30" y="35" font-family="sans-serif" font-size="22">{html.escape(title)}</text>',
             f'<line x1="{x(0)}" y1="60" x2="{x(0)}" y2="325" stroke="#aaa"/>']
    for i, (method, values) in enumerate(zip(methods, groups)):
        y, average = 85 + i * 50, statistics.mean(values)
        parts.extend([f'<text x="175" y="{y + 5}" text-anchor="end" font-family="sans-serif" font-size="16">{method}</text>',
                      f'<line x1="{x(min(values))}" y1="{y}" x2="{x(max(values))}" y2="{y}" stroke="#35748a" stroke-width="3"/>',
                      f'<circle cx="{x(average)}" cy="{y}" r="6" fill="#ab542f"/>',
                      f'<text x="830" y="{y + 5}" font-family="sans-serif" font-size="15">{average:+.2f}%</text>'])
    parts += [f'<text x="{x(low)}" y="345" font-family="sans-serif" font-size="13">{low:.1f}%</text>',
              f'<text x="{x(high)}" y="345" text-anchor="end" font-family="sans-serif" font-size="13">{high:.1f}%</text>',
              '<text x="30" y="380" font-family="sans-serif" font-size="14">Dot = mean; line = min/max across ten seeds. Baselines have one run. Cost = 1 bp per side.</text>', '</svg>']
    output.write_text("\n".join(parts))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    reports = {name: audit(Path("results") / name) for name in ("real", "synthetic")}
    if args.check_only:
        return
    text = ["# Executed results", "", "These results describe the committed datasets and fixed configuration; they are not evidence of a live trading edge.", ""]
    for name, (cfg, scores) in reports.items():
        text += [f"## {name.capitalize()} market example", "", f"Validation-selected policy: **{cfg['selected_policy']}**. Test: {cfg['test_first']} to {cfg['test_last']}.", "",
                 "| Policy | Seeds | Mean net return | Min / max | Mean orders |", "|---|---:|---:|---:|---:|"]
        for method in ("cash", "buy_hold", "momentum", "q_learning", "double_q"):
            group = [x for x in scores if x["phase"] == "test" and x["policy"] == method and float(x["one_way_cost_bps"]) == 1]
            values = [100 * float(x["net_return"]) for x in group]
            text.append(f"| {method} | {len(group)} | {statistics.mean(values):+.2f}% | {min(values):+.2f}% / {max(values):+.2f}% | {statistics.mean(float(x['orders']) for x in group):.1f} |")
        text += ["", f"![{name} returns](../results/{name}/returns.svg)", ""]
        return_chart(scores, Path("results") / name / "returns.svg", f"{name.capitalize()} example: held-out net return")
    text += ["## Interpretation", "", "On real EUR/USD data, validation selects cash. Mean Q-learning test returns are close to zero and below buy-and-hold; mean Double Q-learning returns are negative. On synthetic data, the simple momentum benchmark beats average learned-policy returns. Seed ranges measure training randomness on one market history, not uncertainty across independent market samples.", "",
             "Both learning algorithms solve both fixed mazes optimally in all ten tested seeds. This is training on and evaluating each same maze, not generalization to unseen layouts.", ""]
    Path("docs/RESULTS.md").write_text("\n".join(text))


if __name__ == "__main__":
    main()
