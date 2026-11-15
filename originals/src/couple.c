#include "couple.h"

// Définition externe : number_actions doit être fourni dans chaque contexte (trading ou maze)
extern int number_actions;

couple couple_max(float** Q, int s) {
    float M = Q[s][0];
    int m = 0;

    for (int j = 1; j < number_actions; ++j) {
        if (Q[s][j] > M) {
            M = Q[s][j];
            m = j;
        }
    }

    couple c = {m, M};
    return c;
}
