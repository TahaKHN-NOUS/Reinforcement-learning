CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm
SOURCES = src/agent.c src/maze.c src/data.c src/trading.c

.PHONY: all test sanitize smoke reproduce clean
all: rl
rl: src/main.c $(SOURCES) include/agent.h include/maze.h include/trading.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(SOURCES) $(LDLIBS) -o $@
tests/test_rl: tests/test_rl.c $(SOURCES) include/agent.h include/maze.h include/trading.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_rl.c $(SOURCES) $(LDLIBS) -o $@
test: tests/test_rl
	./tests/test_rl
sanitize:
	$(CC) $(CPPFLAGS) -std=c11 -g -O1 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_rl.c $(SOURCES) $(LDLIBS) -o tests/test_sanitize
	ASAN_OPTIONS=detect_leaks=1 ./tests/test_sanitize
smoke: rl test
	mkdir -p results/smoke
	./rl maze data/maze.txt results/smoke 1
	./rl demo results/smoke/demo.csv
	./rl trade results/smoke/demo.csv results/smoke 1
reproduce: rl test
	mkdir -p results/maze results/maze_small results/real results/synthetic
	./rl maze data/maze.txt results/maze 10
	./rl maze data/maze_small.txt results/maze_small 10
	./rl demo data/synthetic_prices.csv
	./rl trade data/synthetic_prices.csv results/synthetic 10
	./rl trade data/eurusd_daily.csv results/real 10
clean:
	rm -f rl tests/test_rl tests/test_sanitize
