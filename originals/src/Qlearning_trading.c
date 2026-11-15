#include "Qlearning_trading.h"
#include "tradingEnv.h"

float** Q;
int n_states_Q;

void init_Q_trading() {
    Q = malloc(n_states_Q * sizeof(float*));
    for (int i = 0; i < n_states_Q; ++i) {
        Q[i] = malloc(TRADING_ACTIONS * sizeof(float));
        for (int j = 0; j < TRADING_ACTIONS; ++j) {
            Q[i][j] = 0.0;
        }
    }
}
int QL_episode_trading(float alpha, float gamma, float epsilon) {
    tradingEnv_reset();
    int steps = 0;
    tradingOutput out;
    int s = 0;

    while (steps < 250) {
        int a = ((float)rand() / RAND_MAX < epsilon) ? rand() % TRADING_ACTIONS : couple_max(Q, s).m;

        out = tradingEnv_step(a);
        int s_prime = state_to_index(out.new_row, out.new_col);
        couple c = couple_max(Q, s_prime);

        Q[s][a] += alpha * (out.reward + gamma * c.M - Q[s][a]);

        s = s_prime;
        if (out.done) break;
        steps++;
    }

    return steps;
}

void Qlearning1(float alpha, float gamma, float epsilon, int n_episodes) {
    n_states_Q = tradingEnv_total_days() * 2;
    init_Q_trading();

    for (int i = 0; i < n_episodes; ++i) {
        int steps = QL_episode_trading(alpha, gamma, epsilon);
        if ((i+1) % 50 == 0) printf("[Q-learning] Épisode %d terminé en %d étapes\n", i + 1, steps);
    }
}

void Qtrading_free(void) {
    if (!Q) return;

    for (int i = 0; i < n_states_Q; ++i) {
        free(Q[i]);
    }
    free(Q);
    Q = NULL;
}