#include "Qlearning.h"


// Recherche la meilleure action et sa valeur Q à partir d’un état donné s
couple couple_max(float** Q, int s) {
    float M = Q[s][0]; // Initialise avec la première action
    int m = 0;

    // Recherche de la valeur Q maximale parmi les actions possibles
    for (int j = 1; j < number_actions; ++j) {
        if (Q[s][j] > M) {
            M = Q[s][j];
            m = j;
        }
    }

    couple c = {m, M}; // Retourne l'indice et la valeur
    return c;
}

// Politique epsilon-greedy : retourne la probabilité de choisir l'action a dans l’état s
float greedy(double eps, int a, int s, float** Q) {
    couple c = couple_max(Q, s); // Meilleure action à partir de Q-table
    if (c.m == a) {
        return 1 - eps; // Meilleure action : on la choisit avec probabilité 1 - ε
    } else {
        return eps / (number_actions - 1); // Autre action : exploration avec proba uniforme
    }
}

// Politique Boltzmann (softmax) : probabilité de choisir l'action a dans l’état s
float Botzmann(float** Q, int a, int s) {
    float tau = 0;

    // Somme des exponentielles des valeurs Q (normalisation)
    for (int i = 0; i < number_actions; ++i) {
        tau += exp(Q[s][i]);
    }

    // Probabilité d'exploiter l'action a selon softmax
    return exp(Q[s][a]) / tau;
}

// Variables globales
float** Q;     // Q-table dynamique : Q[state][action]
int n_states;  // Nombre total d’états dans l’environnement

// Alloue et initialise la Q-table avec des zéros
void init_Q() {
    Q = malloc(n_states * sizeof(float*)); // Alloue un tableau de lignes (états)
    for (int i = 0; i < n_states; ++i) {
        Q[i] = malloc(number_actions * sizeof(float)); // Alloue les colonnes (actions)
        for (int j = 0; j < number_actions; ++j) {
            Q[i][j] = 0.0; // Initialise à zéro
        }
    }
}

void free_Q() {
    for (int i = 0; i < n_states; ++i) {
        free(Q[i]);
    }
    free(Q);
}

// Convertit une position 2D (ligne, colonne) en un index d’état unique
int state_to_index(int lig, int col) {
    return lig * cols + col; // linéarisation d’un tableau 2D
}

// Exécute un épisode de Q-learning avec epsilon-greedy
int episode(float alpha, float gamma, float epsilon) {
    mazeEnv_reset(); // Replace l’agent à la position de départ
    init_visited();
    int steps = 0;
    envOutput out;

    while (steps < 250) { // Limite max d’itérations pour éviter les boucles infinies
        int s = state_to_index(state_row, state_col); // État actuel

        // Sélection d’action selon la stratégie epsilon-greedy :
        // avec probabilité epsilon, on explore ; sinon, on exploite la meilleure action connue
        int a;
        if ((rand() / (float)RAND_MAX) < epsilon) {
            a = env_action_sample(); // Choix aléatoire (exploration)
        } else {
            couple c = couple_max(Q, s); // Choix optimal (exploitation)
            a = c.m;
        }

        // Applique l'action choisie et observe l'état suivant
        out = mazeEnv_step(a);
        int s_prime = state_to_index(out.new_row, out.new_col);

        // Mise à jour de la Q-table (formule de Q-learning)
        couple c_prime = couple_max(Q, s_prime); // Meilleure action à partir de s'
        Q[s][a] = Q[s][a] + alpha * (out.reward + gamma * c_prime.M - Q[s][a]);

        //mazeEnv_render_pos(); // Affiche la position actuelle de l'agent dans la console
        steps++;

        if (out.done) { // Si l’agent atteint le but
            add_crumbs(); // Ajoute les traces du chemin trouvé
            break;
        }
    }

    return steps;
}

// Fonction principale : lance plusieurs épisodes d’apprentissage avec Q-learning
void Qlearning(float alpha, float gamma, float epsilon, int n_episodes) {
    n_states = rows * cols; // Calcul du nombre total d'états possibles dans le labyrinthe
    init_Q(); // Initialisation de la Q-table

    //dfs(start_row, start_col); // exploration initiale par DFS

    // Boucle sur les épisodes
    for (int i = 0; i < n_episodes; ++i) {
        int steps = episode(alpha, gamma, epsilon);
        if ((i+1) % 50 == 0) printf("L'épisode %d terminé en %d étapes\n", i + 1, steps);
    }
    free_Q();
}
