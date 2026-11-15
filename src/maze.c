#include "maze.h"
#include <stdio.h>
#include <string.h>

int maze_load(Maze *maze, const char *path) {
    FILE *file = fopen(path, "r");
    char line[256];
    int starts = 0, goals = 0;
    if (!file) return 0;
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d,%d", &maze->rows, &maze->cols) != 2 ||
        maze->rows < 1 || maze->cols < 1 || maze->rows > MAX_SIDE || maze->cols > MAX_SIDE) {
        fclose(file); return 0;
    }
    for (int r = 0; r < maze->rows; r++) {
        if (!fgets(line, sizeof(line), file) || (int)strcspn(line, "\r\n") != maze->cols) {
            fclose(file); return 0;
        }
        for (int c = 0; c < maze->cols; c++) {
            char cell = line[c];
            if (cell != '+' && cell != ' ' && cell != 's' && cell != 'g') {
                fclose(file); return 0;
            }
            maze->cells[r][c] = cell;
            if (cell == 's') { maze->start = r * maze->cols + c; starts++; }
            if (cell == 'g') { maze->goal = r * maze->cols + c; goals++; }
        }
    }
    fclose(file);
    return starts == 1 && goals == 1;
}

MazeStep maze_step(const Maze *maze, int state, int action) {
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    MazeStep out = {state, -1, 0};
    if (state == maze->goal) { out.reward = 0; out.done = 1; return out; }
    if (action < 0 || action >= 4) return out;
    int r = state / maze->cols + dr[action];
    int c = state % maze->cols + dc[action];
    if (r >= 0 && r < maze->rows && c >= 0 && c < maze->cols && maze->cells[r][c] != '+') {
        out.state = r * maze->cols + c;
    }
    if (out.state == maze->goal) { out.reward = 50; out.done = 1; }
    return out;
}

int maze_bfs(const Maze *maze) {
    int distance[MAX_STATES], queue[MAX_STATES];
    int head = 0, tail = 0;
    for (int i = 0; i < MAX_STATES; i++) distance[i] = -1;
    distance[maze->start] = 0; queue[tail++] = maze->start;
    while (head < tail) {
        int state = queue[head++];
        if (state == maze->goal) return distance[state];
        for (int a = 0; a < 4; a++) {
            int next = maze_step(maze, state, a).state;
            if (distance[next] == -1) {
                distance[next] = distance[state] + 1; queue[tail++] = next;
            }
        }
    }
    return -1;
}

static int dfs_visit(const Maze *maze, int state, int *seen) {
    if (state == maze->goal) return 0;
    seen[state] = 1;
    for (int a = 0; a < 4; a++) {
        int next = maze_step(maze, state, a).state;
        if (!seen[next]) {
            int length = dfs_visit(maze, next, seen);
            if (length >= 0) return length + 1;
        }
    }
    return -1;
}

int maze_dfs(const Maze *maze) {
    int seen[MAX_STATES] = {0};
    return dfs_visit(maze, maze->start, seen);
}

int maze_evaluate(const Maze *maze, const Agent *agent, int limit, const char *path) {
    char picture[MAX_SIDE][MAX_SIDE];
    memcpy(picture, maze->cells, sizeof(picture));
    int state = maze->start, steps = 0;
    while (state != maze->goal && steps < limit) {
        state = maze_step(maze, state, agent_greedy(agent, state)).state;
        if (state != maze->start && state != maze->goal) picture[state / maze->cols][state % maze->cols] = '.';
        steps++;
    }
    if (path) {
        FILE *file = fopen(path, "w");
        if (!file) return -2;
        fprintf(file, "success=%d steps=%d\n", state == maze->goal, steps);
        for (int r = 0; r < maze->rows; r++) {
            for (int c = 0; c < maze->cols; c++) fputc(picture[r][c], file);
            fputc('\n', file);
        }
        fclose(file);
    }
    return state == maze->goal ? steps : -1;
}

void maze_train(const Maze *maze, Agent *agent, int episodes, unsigned int seed, FILE *curve) {
    for (int ep = 0; ep < episodes; ep++) {
        int state = maze->start;
        double epsilon = 1.0 - 0.95 * ep / (episodes > 1 ? episodes - 1 : 1);
        for (int step = 0; step < 500; step++) {
            int action = agent_action(agent, state, epsilon, &seed);
            MazeStep out = maze_step(maze, state, action);
            /* Step-limit truncation is not an environmental terminal. */
            agent_update(agent, state, action, out.reward, out.state, out.done, 0.2, 0.95, &seed);
            state = out.state;
            if (out.done) break;
        }
        if (curve && (ep == 0 || (ep + 1) % 100 == 0)) {
            fprintf(curve, "%d,%d\n", ep + 1, maze_evaluate(maze, agent, 500, NULL));
        }
    }
}

int maze_experiment(const char *path, const char *output, int seeds) {
    Maze maze;
    Agent agent;
    char filename[512];
    if (!maze_load(&maze, path)) { fprintf(stderr, "Invalid maze: %s\n", path); return 0; }
    snprintf(filename, sizeof(filename), "%s/maze_scores.csv", output);
    FILE *scores = fopen(filename, "w");
    if (!scores) return 0;
    fprintf(scores, "method,seed,success,steps,shortest_steps\n");
    int shortest = maze_bfs(&maze);
    fprintf(scores, "bfs,0,%d,%d,%d\n", shortest >= 0, shortest, shortest);
    int dfs = maze_dfs(&maze);
    fprintf(scores, "dfs,0,%d,%d,%d\n", dfs >= 0, dfs, shortest);
    for (int method = 0; method < 2; method++) {
        const char *name = method ? "double_q" : "q_learning";
        int successes = 0;
        for (int k = 1; k <= seeds; k++) {
            agent_init(&agent, maze.rows * maze.cols, 4, method);
            FILE *curve = NULL;
            if (k == 1) {
                snprintf(filename, sizeof(filename), "%s/maze_%s_curve.csv", output, name);
                curve = fopen(filename, "w");
                if (!curve) { fclose(scores); return 0; }
                fprintf(curve, "episode,greedy_steps_minus1_if_failure\n");
            }
            maze_train(&maze, &agent, 3000, (unsigned int)(11 * k), curve);
            if (curve) fclose(curve);
            int steps = maze_evaluate(&maze, &agent, 500, NULL);
            successes += steps >= 0;
            fprintf(scores, "%s,%d,%d,%d,%d\n", name, 11*k, steps >= 0, steps, shortest);
            if (k == 1) {
                snprintf(filename, sizeof(filename), "%s/maze_%s_path.txt", output, name);
                if (maze_evaluate(&maze, &agent, 500, filename) == -2) { fclose(scores); return 0; }
            }
        }
        printf("Maze %s: %d/%d successful frozen policies; BFS optimum %d\n", name, successes, seeds, shortest);
    }
    fclose(scores);
    return 1;
}
