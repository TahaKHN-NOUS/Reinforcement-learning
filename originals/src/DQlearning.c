/*
#include "DQlearning.h"
#include "mazeEnv.h"
#include "functions.h"
#include "dfs.h"
#include "Qlearning.h"
#include <stdlib.h>
#include <stdio.h>


float** Q1;
float** Q2;
int n_states_DQ;

// Initialisation de deux Q-tables
void init_DQ() {
    Q1 = malloc(n_states_DQ * sizeof(float*));
    Q2 = malloc(n_states_DQ * sizeof(float*));
    for (int i = 0; i < n_states_DQ; ++i) {
        Q1[i] = malloc(number_actions * sizeof(float));
        Q2[i] = malloc(number_actions * sizeof(float));
        for (int j = 0; j < number_actions; ++j) {
            Q1[i][j] = 0.0;
            Q2[i][j] = 0.0;
        }
    }
}

int DQ_episode(float alpha, float gamma, float epsilon) {
    mazeEnv_reset();
    int steps = 0;
    envOutput out;

    while (steps < 250) {
        int s = state_to_index(state_row, state_col);

        int a;
        if ((rand() / (float)RAND_MAX) < epsilon) {
            a = env_action_sample();
        } else {
            float* Q_sum = malloc(number_actions * sizeof(float));
            for (int i = 0; i < number_actions; ++i)
                Q_sum[i] = Q1[s][i] + Q2[s][i];

            couple c = couple_max(&Q_sum, s);  // Pass address to simulate pointer access
            a = c.m;
            free(Q_sum);
        }

        out = mazeEnv_step(a);
        int s_prime = state_to_index(out.new_row, out.new_col);

        if ((rand() / (float)RAND_MAX) < 0.5) {
            couple c = couple_max(Q1, s_prime);
            Q1[s][a] += alpha * (out.reward + gamma * Q2[s_prime][c.m] - Q1[s][a]);
        } else {
            couple c = couple_max(Q2, s_prime);
            Q2[s][a] += alpha * (out.reward + gamma * Q1[s_prime][c.m] - Q2[s][a]);
        }

        mazeEnv_render_pos();
        steps++;

        if (out.done) {
            add_crumbs();
            break;
        }
    }

    return steps;
}

void DQlearning(float alpha, float gamma, float epsilon, int n_episodes) {
    n_states_DQ = rows * cols;
    init_DQ();
    

    for (int i = 0; i < n_episodes; ++i) {
        int steps = DQ_episode(alpha, gamma, epsilon);
        printf("Double Q-learning épisode %d terminé en %d étapes\n", i + 1, steps);
    }
}
*/
#include "DQlearning.h"
#include "mazeEnv.h"
#include "functions.h"
#include "dfs.h"
#include "Qlearning.h"
#include <stdlib.h>
#include <stdio.h>

float** Q1;
float** Q2;
int n_states_DQ;

// Cherche l'action optimale dans Q1 + Q2
couple couple_max_sum(float** Q1, float** Q2, int s) {
    float M = Q1[s][0] + Q2[s][0];
    int m = 0;
    for (int j = 1; j < number_actions; ++j) {
        float sum = Q1[s][j] + Q2[s][j];
        if (sum > M) {
            M = sum;
            m = j;
        }
    }
    couple c = {m, M};
    return c;
}

void init_DQ() {
    Q1 = malloc(n_states_DQ * sizeof(float*));
    Q2 = malloc(n_states_DQ * sizeof(float*));
    if (!Q1 || !Q2) {
        printf("Erreur d'allocation de Q1 ou Q2\n");
        exit(1);
    }

    for (int i = 0; i < n_states_DQ; ++i) {
        Q1[i] = malloc(number_actions * sizeof(float));
        Q2[i] = malloc(number_actions * sizeof(float));
        if (!Q1[i] || !Q2[i]) {
            printf("Erreur d'allocation pour Q1[%d] ou Q2[%d]\n", i, i);
            exit(1);
        }
        for (int j = 0; j < number_actions; ++j) {
            Q1[i][j] = 0.0;
            Q2[i][j] = 0.0;
        }
    }
}

int DQ_episode(float alpha, float gamma, float epsilon) {
    mazeEnv_reset();
    init_visited();
    int steps = 0;
    envOutput out;

    while (steps < 250) {
        int s = state_to_index(state_row, state_col);

        int a;
        if ((rand() / (float)RAND_MAX) < epsilon) {
            a = env_action_sample();
        } else {
            couple c = couple_max_sum(Q1, Q2, s);
            a = c.m;
        }

        out = mazeEnv_step(a);
        int s_prime = state_to_index(out.new_row, out.new_col);

        if ((rand() / (float)RAND_MAX) < 0.5) {
            couple c = couple_max(Q1, s_prime);
            Q1[s][a] += alpha * (out.reward + gamma * Q2[s_prime][c.m] - Q1[s][a]);
        } else {
            couple c = couple_max(Q2, s_prime);
            Q2[s][a] += alpha * (out.reward + gamma * Q1[s_prime][c.m] - Q2[s][a]);
        }

        steps++;

        if (out.done) {
            add_crumbs();
            break;
        }
    }

    return steps;
}

void DQlearning(float alpha, float gamma, float epsilon, int n_episodes) {
    n_states_DQ = rows * cols;
    init_DQ();

    for (int i = 0; i < n_episodes; ++i) {
        int steps = DQ_episode(alpha, gamma, epsilon);
        if ((i+1) % 50 == 0) printf("Double Q-learning épisode %d terminé en %d étapes\n", i + 1, steps);
    }
    free_DQ();
}

void free_DQ(){
    for (int i = 0; i < n_states_DQ; ++i) {
        free(Q1[i]);
        free(Q2[i]);
    }
    free(Q1);
    free(Q2);
}