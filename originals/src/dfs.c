#include <stdio.h>
#include "dfs.h"
#include "Qlearning.h"
#include "DQlearning.h"

int dfs(int row, int col){
    int* current = &visited[row][col];

    if(*current == goal){
        return 1;
    }else if(*current == unknown){
        *current = known;

        if (dfs(row, col-1)){
            *current = crumb;
            return 1;
        } else if (dfs(row+1, col)){
            *current = crumb;
            return 1;
        }  else if (dfs(row, col+1)){
            *current = crumb;
            return 1;
        }  else if (dfs(row-1, col)){
            *current = crumb;
            return 1;
        }

    }

    return 0;
}

int main() {
    printf("Chargement du labyrinthe\n");
    mazeEnv_make("data/maze.txt");
    init_visited();

    // Phase 1 : DFS pour exploration basique
    printf("\nRésolution par DFS\n");
    mazeEnv_render();
    dfs(start_row, start_col);
    add_crumbs();
    mazeEnv_render();

    // Phase 2 : Début d'intégration du Q-learning
    printf("\nDébut du Q-learning (test sur 500 épisodes)\n");
    Qlearning(0.1, 0.9, 0.001,500);  // alpha, gamma, epsilon, nb d'épisodes
    mazeEnv_render();
    // Phase 3 : Début d'intégration du DQ-learning
    printf("\nDébut du Double DQ-learning (test sur 500 épisodes)\n");
    init_visited();
    DQlearning(0.1, 0.9, 0.001, 500);  // alpha, gamma, epsilon, nb d'épisodes
    mazeEnv_render();
    return 0;
}

