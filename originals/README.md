# Reinforcement Learning Project – Maze Escape & Trading

Welcome to our repository dedicated to Reinforcement Learning, created as part of a course project at ENSTA Paris.

## Goal

Our objective is to implement and test Reinforcement Learning (RL) algorithms to solve two problems:

1. **Maze Escape** – The agent must learn to escape a maze from a starting point to a goal.
2. **Trading Environment** – The agent must learn a trading strategy (Buy/Sell/Hold) to maximize profit over time based on historical price data.

Currently, we are focusing on the following RL algorithms:

* **Q-learning**, a model-free RL technique.
* **Double Q-learning (DQLearning)**, which mitigates overestimation bias in Q-learning.

---

## Repository Structure

* `include/`        – Header files (e.g., `mazeEnv.h`, `functions.h`, `Qlearning.h`, `dfs.h`, `tradingEnv.h`, `Qlearning_trading.h`, `DQlearning.h`)
* `src/`            – Source files (e.g., `mazeEnv.c`, `dfs.c`, `Qlearning.c`, `functions.c`, `tradingEnv.c`, `Qlearning_trading.c`, `DQlearning.c`)
* `data/`           – Maze structure and financial data files (`maze.txt`, `train.txt`, `real.txt`)
* `Makefile`        – Build instructions
* `README.md`       – Project documentation

---

## Folder Details

### include/

Contains header files defining interfaces for all environments and learning algorithms:

* `mazeEnv.h`: Handles maze environment setup and agent movement
* `functions.h`: Utility functions (e.g., min, max)
* `Qlearning.h`: Q-learning for maze
* `dfs.h`: DFS algorithm for maze pathfinding
* `tradingEnv.h`: Trading environment with price data and trading logic
* `Qlearning_trading.h`: Q-learning adaptation for the trading scenario
* `DQlearning.h`: Double Q-learning for the trading environment

### src/

Includes implementations for both environments:

* Maze Escape:

  * `mazeEnv.c`: Environment logic
  * `dfs.c`: Depth-first search for comparison
  * `Qlearning.c`: Q-learning logic for maze
  * `DQlearning.c`: Double Q-learning variant for maze
* Trading:

  * `tradingEnv.c`: Trading environment (price tracking, actions, rewards)
  * `Qlearning_trading.c`: Q-learning agent adapted for trading
  

### data/

Contains all input files:

* `maze.txt`: Maze layout (grid size + symbols like '+', 's', 'g')
* `train.txt`: Historical training prices for trading environment
* `real.txt`: Real price used to compare the prediction

---

## Makefile

Used to compile all components of the project and generate the executable. Supports compilation of both the maze and trading modules.

---

## Usage

You can switch between running the **maze** RL problem or the **trading** RL problem by adjusting the main function or the target in the Makefile.

---

## Authors

Developed by students of ENSTA Paris KHAZZAKA Jude Nemer and KHN-NOUS Taha as part of the Reinforcement Learning course project.