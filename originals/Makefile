CC = gcc                                # Compilateur
CFLAGS = -Wall -Werror -Iinclude -g     # Options de compilation
LDFLAGS = -lm                           # Linker flags (math)


TRADING_SRCS = src/main.c src/tradingEnv.c src/Qlearning_trading.c src/couple.c
TRADING_OBJS = $(TRADING_SRCS:.c=.o)
TRADING_TARGET = main.x


DFS_SRCS = src/dfs.c src/functions.c src/mazeEnv.c src/Qlearning.c src/DQlearning.c
DFS_OBJS = $(DFS_SRCS:.c=.o)
DFS_TARGET = src/dfs


all: $(TRADING_TARGET) $(DFS_TARGET)


$(TRADING_TARGET): $(TRADING_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


$(DFS_TARGET): $(DFS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o

realclean: clean
	rm -f $(TRADING_TARGET) $(DFS_TARGET)
