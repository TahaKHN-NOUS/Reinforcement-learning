#include "tradingEnv.h"
#include "couple.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DAYS 1000

int number_actions = 3;

const int number_actions_trading = TRADING_ACTIONS;

static float prices[MAX_DAYS];
static int current_day;
static int total_days;
static int position; // 0 = no position, 1 = bought
static float buy_price;

void tradingEnv_make(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erreur ouverture fichier train.txt");
        exit(EXIT_FAILURE);
    }

    total_days = 0;
    while (fscanf(file, "%f", &prices[total_days]) == 1 && total_days < MAX_DAYS) {
        total_days++;
    }
    fclose(file);
}

void tradingEnv_reset() {
    current_day = 0;
    position = 0;
    buy_price = 0.0;
}

// Discrétisation de l'état : (jour, position)
int state_to_index(int day, int pos) {
    return day * 2 + pos;
}

tradingOutput tradingEnv_step(int action) {
    tradingOutput out;
    out.done = 0;
    out.reward = 0.0;
    out.real_price_J = 0.0;

    if (current_day + 1 >= total_days) {
        out.done = 1;
        out.new_row = current_day;
        out.new_col = position;
        return out;
    }

    float price_today = prices[current_day];
    float price_tomorrow = prices[current_day + 1];
    out.real_price_J = price_tomorrow;

    // Action: 0 = BUY, 1 = SELL, 2 = HOLD
    if (action == 0 && position == 0) {
        position = 1;
        buy_price = price_today;
        out.reward = -0.1;
    } else if (action == 1 && position == 1) {
        position = 0;
        float gain = price_tomorrow - buy_price;
        out.reward = gain - 0.5;
    } else {
        out.reward = -0.1;
    }

    current_day++;
    out.new_row = current_day;
    out.new_col = position;

    return out;
}

float tradingEnv_predict(float** Q, int last_state) {
    int action = couple_max(Q, last_state).m;
    float last_price = prices[total_days - 1];

    if (action == 0) return last_price * 1.01;  // BUY → estimation haussière
    if (action == 1) return last_price * 0.99;  // SELL → estimation baissière
    return last_price;                         // HOLD → estimation stable
}

float load_real_price(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Erreur d'ouverture de real.txt");
        exit(EXIT_FAILURE);
    }

    float real_price;
    fscanf(f, "%f", &real_price);
    fclose(f);
    return real_price;
}

int tradingEnv_total_days(void) {
    return total_days;
}