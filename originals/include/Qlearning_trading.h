#include "tradingEnv.h"
#include <stdlib.h>
#include <stdio.h>
#ifndef QLEARNING_TRADING_H
#define QLEARNING_TRADING_H


void init_Q_trading();
int QL_episode_trading(float alpha, float gamma, float epsilon);
void Qlearning1(float alpha, float gamma, float epsilon, int n_episodes);

#endif
