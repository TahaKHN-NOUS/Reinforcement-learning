#ifndef MAZE_H
#define MAZE_H
#include "agent.h"
#include <stdio.h>
#define MAX_SIDE 40

typedef struct {
    int rows, cols, start, goal;
    char cells[MAX_SIDE][MAX_SIDE];
} Maze;

typedef struct {
    int state;
    double reward;
    int done;
} MazeStep;

int maze_load(Maze *maze, const char *path);
MazeStep maze_step(const Maze *maze, int state, int action);
int maze_bfs(const Maze *maze);
int maze_dfs(const Maze *maze);
int maze_evaluate(const Maze *maze, const Agent *agent, int limit, const char *path);
void maze_train(const Maze *maze, Agent *agent, int episodes, unsigned int seed, FILE *curve);
int maze_experiment(const char *path, const char *output, int seeds);
#endif
