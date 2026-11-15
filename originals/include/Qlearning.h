#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mazeEnv.h"
#include "functions.h"
#include "dfs.h"
#ifndef QLEARNING_H
#define QLEARNING_H
#include "couple.h"

#define number_actions 4


couple couple_max(float** Q, int s);

void init_Q();

void free_Q();

int state_to_index(int row, int col);


int episode(float alpha, float gamma, float epsilon);


void Qlearning(float alpha, float gamma, float epsilon, int n_episodes);

#endif 

