# Method and equations

## Agent and learning

Both problems use a finite state/action table. Values initialize to zero.
Training uses epsilon-greedy actions, with epsilon declining linearly from
1 to 0.05 over episodes. Greedy ties are randomized during training and
resolved deterministically during evaluation. Market states never visited
in training use a flat-exposure fallback during evaluation and inference.

Q-learning updates

$$
Q(s_t,a_t)\leftarrow Q(s_t,a_t)+\eta\,[y_t-Q(s_t,a_t)],
\qquad
y_t=\begin{cases}
r_t & \text{terminal},\\
r_t+\gamma\max_a Q(s_{t+1},a)&\text{otherwise}.
\end{cases}
$$

Double Q-learning maintains independent tables A and B. At each transition,
one table is randomly selected for updating. For an update to A:

$$
a^*=\operatorname*{arg\,max}_a Q_A(s_{t+1},a),\qquad
y_t=r_t+\gamma Q_B(s_{t+1},a^*),
$$

with the same terminal override. For an update to B, swap the table roles.
Behavioral greedy action selection uses $Q_A+Q_B$. Double Q-learning reduces
a source of maximization bias; it does not guarantee higher trading returns
or faster learning on every example.

Randomness uses an explicit 32-bit linear congruential generator with state
passed by pointer; overflow is defined unsigned arithmetic. Fixed seeds are
11, 22, ..., 110. No global random state is shared between experiments.

## Maze

State is $s=r\cdot\mathrm{cols}+c$. Movement outside the grid or into a wall
leaves the agent in place. A non-goal move receives −1 and reaching the goal
receives +50. The goal is absorbing with zero further reward.

Each policy trains for 3,000 episodes, at most 500 actions per episode,
using learning rate 0.2 and discount 0.95. Hitting the artificial step limit
truncates a training trajectory but does not create a true terminal state
for its Bellman target. Evaluation has no exploration or learning; failure
to reach the goal in 500 actions is recorded as −1 steps and success=0.

BFS supplies the shortest-path reference. DFS supplies a valid route, which
need not be shortest. Every algorithm uses the original immutable maze.
Rendered learned routes come from frozen policy rollouts, not DFS markings.
The two fixed mazes test in-layout learning, not unseen-layout generalization.

## Market observations and state

Rows contain observed-session date, first observed open $O_t$, and last
observed close $C_t$. At execution open $t$, features use closes through $t-1$:

$$
u_k=\log(C_k/C_{k-1}),\qquad
m_k=\log(C_k/C_{k-5}),\qquad
v_k=\sqrt{\frac1{20}\sum_{j=k-19}^{k}u_j^2},\qquad k=t-1.
$$

Volatility is RMS return, not demeaned sample standard deviation. The first
action index is 21, providing 20 historical returns. Returns and momentum
have three bins; volatility has two. For $n$ sorted training feature values,
return/momentum cuts are elements floor(n/3) and floor(2n/3), using zero-based
indexing. The volatility cut is element floor(n/2). Boundaries are fitted
from training closes only and remain fixed thereafter.

With bins $b_u,b_m\in\{0,1,2\}$, $b_v\in\{0,1\}$, and position
$p\in\{0,1\}$:

$$
s=2\,[2(3b_u+b_m)+b_v]+p\in\{0,\ldots,35\}.
$$

These coarse features do not guarantee Markov market dynamics. Unlike the
original day-number state, they can recur in later periods. Position suffices
for the simplified portfolio accounting because rewards no longer depend on
an unrecorded purchase price. There is no short selling or leverage.

## Actions, execution, accounting, and rewards

BUY targets $p'_t=1$, SELL targets $p'_t=0$, and HOLD keeps $p'_t=p_t$.
Acting after the prior observed close executes at the next observed session's
open. The position then earns the open-to-next-open price return:

$$
R_t=\frac{O_{t+1}}{O_t}-1.
$$

Let $c=\mathrm{one\_way\_cost\_bps}/10^4$, with default $c=10^{-4}$.
Starting from wealth $W_t$, turnover is $\tau_t=|p'_t-p_t|$ and the fees are

$$
F_t=W_t c\tau_t,\qquad
W^-_{t+1}=(W_t-F_t)(1+p'_tR_t).
$$

On the final transition, liquidate at its ending open:

$$
L_t=W^-_{t+1}cp'_t I_{\mathrm{terminal}},\qquad
W_{t+1}=W^-_{t+1}-L_t.
$$

Reward is

$$
r_t=100\log(W_{t+1}/W_t).
$$

Log reward is wealth-scale invariant. Multiplication by 100 keeps numerical
values readable; it does not turn Q values into price forecasts. Training
optimizes discounted log rewards, while validation uses final log wealth;
these objectives are related but not identical when discount is below one.

No entry fee is charged when exposure remains unchanged. Every completed
evaluation begins and ends flat. An order is an actual exposure change;
terminal liquidation counts as a sell order. This differs from forcing a
roundtrip on every bar. Initial reference capital is 10,000 account units.

The gross-return counterfactual follows the same positions with no fees.
Fees paid are summed in account currency, not naively subtracted from a
compounded percentage return. Financing, cash interest, FX interest-rate
differentials, spread variation, market impact, and order-fill uncertainty
are not modeled. Costs and execution opens are assumptions.

## Chronological experiment and selection

For $N$ rows, let $T=\lfloor0.6N\rfloor$ and $V=\lfloor0.8N\rfloor$:

- Train actions: indices 21 through T−2, with terminal liquidation at open T−1.
- Validation actions: T through V−2, liquidating at open V−1.
- Test actions: V through N−2, liquidating at open N−1.

Each segment starts flat; intervals crossing segment boundaries are excluded.
Earlier observed closes may supply feature history. Every learned table uses
400 training episodes with learning rate 0.1, discount 0.95, and the fixed
exploration schedule. Policies do not update during validation or test.
Test features can use earlier test observations already available in time.
There is no hyperparameter search or test-based model tuning.

For each learned method, selection computes

$$
S_{\mathrm{method}}=\frac1{10}\sum_{k=1}^{10}
\log\left(W^{\mathrm{validation}}_{k,\mathrm{end}}/10000\right).
$$

Compare these with cash's score zero. The maximum chooses the method before
the test is evaluated, with cash winning a zero-score tie. Buy-and-hold and
positive five-session-momentum are additional reported benchmarks, not
selection candidates. Each learned method is reported across all ten seeds.
The persisted illustrative policy uses predeclared seed 11, not the best
validation or test seed. If selected cash, the selected ledger is cash.

Cost sensitivity reuses the same trained agents at 0, 0.5, 1, 2, and 5 bp per
side. Features and policy actions do not depend on wealth or evaluation cost,
so cost scenarios do not introduce a new fit or a different signal rule.

## Metrics and interpretation

Total net return is $W_{\mathrm{end}}/10000-1$. Maximum drawdown is

$$
\mathrm{MDD}=\min_t\left(W_t/\max_{u\leq t}W_u-1\right),
$$

including initial wealth. The reported observed-session Sharpe convention is

$$
\mathrm{Sharpe}_{252}=\sqrt{252}\,\overline{R^{net}}/s(R^{net}),
\qquad R^{net}_t=W_{t+1}/W_t-1.
$$

It uses sample standard deviation, zero risk-free rate, and becomes undefined
for zero volatility. Because observations can span weekends, holidays, and
missing dates, 252 is a reporting convention, not an exact calendar adjustment.
Drawdown is sampled at observed opens and can miss intraperiod losses.
No CAGR or significance claim is used to disguise those limitations.

Ten seeds measure training randomness on the same price history. They are
not ten independent financial samples. A positive test average alone does
not establish statistical significance, economic superiority, or live
tradability. The selected cash result and unfavorable benchmark comparisons
are part of the study, not errors to tune away.
