#include "agent.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

double random_number(unsigned int *seed) {
    uint32_t next = (uint32_t)*seed * UINT32_C(1664525) + UINT32_C(1013904223);
    *seed = (unsigned int)next;
    return next / 4294967296.0;
}

void agent_init(Agent *agent, int states, int actions, int double_q) {
    assert(states > 0 && states <= MAX_STATES);
    assert(actions > 0 && actions <= MAX_ACTIONS);
    memset(agent, 0, sizeof(*agent));
    agent->states = states;
    agent->actions = actions;
    agent->double_q = double_q;
}

static double value(const Agent *agent, int s, int a) {
    return agent->q1[s][a] + (agent->double_q ? agent->q2[s][a] : 0);
}

int agent_greedy(const Agent *agent, int state) {
    int best = 0;
    assert(state >= 0 && state < agent->states);
    for (int a = 1; a < agent->actions; a++) {
        if (value(agent, state, a) > value(agent, state, best)) best = a;
    }
    return best;
}

int agent_action(const Agent *agent, int state, double epsilon, unsigned int *seed) {
    if (random_number(seed) < epsilon) {
        return (int)(random_number(seed) * agent->actions);
    }
    /* Random tie-breaking during training avoids always choosing action zero. */
    int best = 0;
    int ties = 1;
    for (int a = 1; a < agent->actions; a++) {
        double v = value(agent, state, a);
        double b = value(agent, state, best);
        if (v > b) { best = a; ties = 1; }
        else if (v == b) {
            ties++;
            if (random_number(seed) < 1.0 / ties) best = a;
        }
    }
    return best;
}

void agent_update(Agent *agent, int s, int a, double reward, int next, int done,
                  double alpha, double gamma, unsigned int *seed) {
    double target = reward;
    assert(s >= 0 && s < agent->states && a >= 0 && a < agent->actions);
    assert(done || (next >= 0 && next < agent->states));
    agent->visits[s]++;
    if (!agent->double_q) {
        if (!done) target += gamma * agent->q1[next][agent_greedy(agent, next)];
        agent->q1[s][a] += alpha * (target - agent->q1[s][a]);
    } else {
        double (*selected)[MAX_ACTIONS];
        double (*other)[MAX_ACTIONS];
        if (random_number(seed) < 0.5) {
            selected = agent->q1; other = agent->q2;
        } else {
            selected = agent->q2; other = agent->q1;
        }
        if (!done) {
            int best = 0;
            for (int j = 1; j < agent->actions; j++) {
                if (selected[next][j] > selected[next][best]) best = j;
            }
            target += gamma * other[next][best];
        }
        selected[s][a] += alpha * (target - selected[s][a]);
    }
}

int agent_save(const Agent *agent, const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) return 0;
    fprintf(file, "TABULAR_RL_2 %d %d %d\n", agent->states, agent->actions, agent->double_q);
    for (int s = 0; s < agent->states; s++) {
        fprintf(file, "%d\n", agent->visits[s]);
        for (int a = 0; a < agent->actions; a++) {
            fprintf(file, "%.17g %.17g\n", agent->q1[s][a], agent->q2[s][a]);
        }
    }
    return fclose(file) == 0;
}

int agent_load(Agent *agent, const char *path) {
    int states, actions, double_q;
    char magic[32];
    FILE *file = fopen(path, "r");
    if (!file) return 0;
    if (fscanf(file, "%31s %d %d %d", magic, &states, &actions, &double_q) != 4 ||
        strcmp(magic, "TABULAR_RL_2") || states < 1 || states > MAX_STATES ||
        actions < 1 || actions > MAX_ACTIONS || (double_q != 0 && double_q != 1)) {
        fclose(file); return 0;
    }
    agent_init(agent, states, actions, double_q);
    for (int s = 0; s < states; s++) {
        if (fscanf(file, "%d", &agent->visits[s]) != 1 || agent->visits[s] < 0) {fclose(file);return 0;}
        for (int a = 0; a < actions; a++) {
            if (fscanf(file, "%lf %lf", &agent->q1[s][a], &agent->q2[s][a]) != 2 ||
                !isfinite(agent->q1[s][a]) || !isfinite(agent->q2[s][a])) {
                fclose(file); return 0;
            }
        }
    }
    fclose(file);
    return 1;
}
