#ifndef AGENT_H
#define AGENT_H

#define MAX_STATES 1600
#define MAX_ACTIONS 4

typedef struct {
    double q1[MAX_STATES][MAX_ACTIONS];
    double q2[MAX_STATES][MAX_ACTIONS];
    int visits[MAX_STATES];
    int states;
    int actions;
    int double_q;
} Agent;

double random_number(unsigned int *seed);
void agent_init(Agent *agent, int states, int actions, int double_q);
int agent_greedy(const Agent *agent, int state);
int agent_action(const Agent *agent, int state, double epsilon, unsigned int *seed);
void agent_update(Agent *agent, int state, int action, double reward,
                  int next_state, int done, double alpha, double gamma,
                  unsigned int *seed);
int agent_save(const Agent *agent, const char *path);
int agent_load(Agent *agent, const char *path);
#endif
