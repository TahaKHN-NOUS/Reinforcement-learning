#include "Qlearning_trading.h"
#include "tradingEnv.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main() {
    srand(0);
    printf("Début de l'apprentissage Q-learning pour le CAC40\n");

    // Chargement des données d'entraînement
    tradingEnv_make("data/train.txt");

    // Initialisation et apprentissage
    int episodes = 500;
    float alpha = 0.1;
    float gamma = 0.99;
    float epsilon = 0.1;
    extern float** Q;
    Qlearning1(alpha, gamma, epsilon, episodes);

    // Dernier état atteint
    int last_day = tradingEnv_total_days() - 1;
    int last_state = state_to_index(last_day, 0);

    // Prédiction de l'agent pour le 30 mai
    float predicted = tradingEnv_predict(Q, last_state); // Q-table interne utilisée

    // Chargement de la vraie valeur du 30 mai
    float real = load_real_price("data/real.txt");

    // Calcul de l'erreur relative
    float error = fabs(predicted - real) / real;

    printf("\n===== Résultat Prédiction CAC40 =====\n");
    printf("Prédiction de l'agent (30 mai) : %.2f\n", predicted);
    printf("Valeur réelle (30 mai)         : %.2f\n", real);
    printf("Erreur relative                : %.2f%%\n", error * 100);

    Qtrading_free();

    return 0;
}

