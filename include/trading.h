#ifndef TRADING_H
#define TRADING_H
#include "agent.h"
#include <stdio.h>
#define MAX_BARS 10000
#define TRADING_STATES 36
#define WARMUP 21

enum TradeAction { BUY, SELL, HOLD };
enum Policy { POLICY_Q, POLICY_DOUBLE_Q, CASH, BUY_HOLD, MOMENTUM };

typedef struct { char date[16]; double open, close; } Bar;
typedef struct { Bar bars[MAX_BARS]; int count; } Market;
typedef struct { double return_low, return_high, momentum_low, momentum_high, volatility_mid; } Cuts;
typedef struct { double daily_return, momentum, volatility; } Features;
typedef struct {
    int position, next_position, done;
    double gross_factor, net_factor, trade_cost_fraction, liquidation_fraction;
} TradeStep;
typedef struct {
    double total_return, gross_return, sharpe, drawdown, fees, log_growth;
    int orders, periods;
} Performance;

int market_load(Market *market, const char *path);
int market_prepare(const char *input, const char *output);
int market_demo(const char *path, unsigned int seed);
Features market_features(const Market *market, int previous_bar);
Cuts market_fit_cuts(const Market *market, int training_end);
int trading_state(const Market *market, const Cuts *cuts, int time, int position);
TradeStep trading_step(const Market *market, int time, int end, int position, int action, double cost_bps);
void trading_train(const Market *market, const Cuts *cuts, Agent *agent,
                   int end, int episodes, unsigned int seed, double cost_bps, FILE *curve);
Performance trading_evaluate(const Market *market, const Cuts *cuts, const Agent *agent,
                             int policy, int start, int end, double cost_bps, FILE *ledger);
int trading_signal(const char *path, const char *model_directory, int position);
int trading_experiment(const char *path, const char *output, int seeds);
#endif
