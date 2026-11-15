#ifndef COUPLE_H
#define COUPLE_H

// Structure utilisée pour stocker la meilleure action (m) et sa valeur Q (M)
typedef struct {
    int m;   // action index
    float M; // valeur maximale
} couple;

// Fonction utilitaire pour trouver l'action ayant la meilleure valeur Q
couple couple_max(float** Q, int s);

#endif
