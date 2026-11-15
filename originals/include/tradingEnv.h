#include "couple.h"

#ifndef TRADING_ENV_H
#define TRADING_ENV_H

#define TRADING_ACTIONS 3
extern const int number_actions_trading;


couple couple_max(float** Q, int s);
typedef struct {
    int new_row;
    int new_col;
    float reward;
    int done;
    float real_price_J;
} tradingOutput;

void tradingEnv_make(const char* filename);
void tradingEnv_reset();
int state_to_index(int day, int pos);
tradingOutput tradingEnv_step(int action);
float tradingEnv_predict(float** Q, int last_state);
float load_real_price(const char* filename);
int tradingEnv_total_days(void);
void Qtrading_free(void);

#endif


