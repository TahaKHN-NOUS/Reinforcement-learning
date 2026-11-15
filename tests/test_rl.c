#include "agent.h"
#include "maze.h"
#include "trading.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks=0;
static void check(int condition,const char *name) {
    if(!condition) {fprintf(stderr,"FAIL: %s\n",name);exit(1);}
    checks++;printf("PASS: %s\n",name);
}
static int close_enough(double a,double b) {return fabs(a-b)<1e-10;}

int main(void) {
    Agent agent,loaded;
    unsigned int seed=11;
    agent_init(&agent,4,3,0);
    agent.q1[1][2]=100;
    agent_update(&agent,0,0,2,1,1,.5,.9,&seed);
    check(close_enough(agent.q1[0][0],1),"terminal Q target excludes future values");
    agent_update(&agent,0,1,2,1,0,.5,.9,&seed);
    check(close_enough(agent.q1[0][1],46),"nonterminal Q update");
    agent_init(&agent,4,3,1);
    agent.q1[1][0]=10;agent.q1[1][1]=2;
    agent.q2[1][0]=1;agent.q2[1][1]=20;
    unsigned int probe=seed;double draw=random_number(&probe);
    agent_update(&agent,0,0,2,1,0,1,.5,&seed);
    check(close_enough(draw<.5 ? agent.q1[0][0] : agent.q2[0][0],draw<.5 ? 2.5 : 3),
          "Double Q selects with one table and evaluates with the other");
    agent_update(&agent,0,2,-3,1,1,1,.9,&seed);
    check(close_enough(agent.q1[0][2]+agent.q2[0][2],-3),"terminal Double Q target");
    check(agent_save(&agent,"tests/policy.tmp") && agent_load(&loaded,"tests/policy.tmp") &&
          !memcmp(&agent,&loaded,sizeof(agent)),"saved Q tables reload exactly");
    remove("tests/policy.tmp");
    agent_init(&agent,4,3,0);
    int seen[3]={0};
    for(int i=0;i<1000;i++) seen[agent_action(&agent,0,0,&seed)]++;
    check(seen[0]>0 && seen[1]>0 && seen[2]>0,"training breaks zero-table ties across actions");
    Maze maze;
    check(maze_load(&maze,"data/maze.txt") && maze_bfs(&maze)==45,"original maze BFS shortest path");
    check(maze_step(&maze,0,0).state==0 && maze_step(&maze,0,2).state==0,"outer-border moves cannot read outside the grid");
    MazeStep goal=maze_step(&maze,maze.goal,0);
    check(goal.done && goal.reward==0 && goal.state==maze.goal,"goal is absorbing");
    check(maze_load(&maze,"data/maze_small.txt") && maze_bfs(&maze)==10,"second maze shortest path");
    char original[MAX_SIDE][MAX_SIDE];memcpy(original,maze.cells,sizeof(original));
    agent_init(&agent,maze.rows*maze.cols,4,0);maze_train(&maze,&agent,3000,11,NULL);
    check(maze_evaluate(&maze,&agent,500,NULL)==10,"trained frozen maze policy reaches shortest route");
    check(!memcmp(original,maze.cells,sizeof(original)),"training and rendering do not mutate the maze");
    Market *market=malloc(sizeof(Market));
    Market *changed=malloc(sizeof(Market));
    check(market && changed,"test data allocation");
    check(market_demo("tests/demo.tmp",12345) && market_load(market,"tests/demo.tmp"),"synthetic dataset is generated and validated");
    check(market->count==3000 && !strcmp(market->bars[0].date,"2000-01-03"),"deterministic dated synthetic input");
    Cuts cuts=market_fit_cuts(market,1800);
    *changed=*market;
    for(int t=1800;t<changed->count;t++) {changed->bars[t].open*=2;changed->bars[t].close*=2;}
    Cuts other=market_fit_cuts(changed,1800);
    check(!memcmp(&cuts,&other,sizeof(cuts)),"future prices cannot change training thresholds");
    check(trading_state(market,&cuts,1800,0)==trading_state(changed,&cuts,1800,0),"current and future bars cannot change the pre-open state");
    int min=100,max=-1;
    for(int t=21;t<market->count;t++) for(int p=0;p<2;p++) {
        int state=trading_state(market,&cuts,t,p);if(state<min)min=state;if(state>max)max=state;
    }
    check(min>=0 && max<TRADING_STATES,"feature bins stay inside the 36-state table");
    int s=trading_state(market,&cuts,1800,0);
    check(trading_state(market,&cuts,1800,1)==s+1,"state explicitly includes exposure");
    market->bars[22].open=100;market->bars[23].open=110;
    TradeStep step=trading_step(market,22,24,0,BUY,1);
    check(step.done && step.next_position==0 && close_enough(step.net_factor,.9999*1.1*.9999),
          "buy, open-to-open return, and terminal liquidation compound correctly");
    step=trading_step(market,22,25,1,BUY,1);
    check(step.trade_cost_fraction==0 && step.liquidation_fraction==0,"holding exposure does not pay a new entry fee");
    step=trading_step(market,22,25,1,SELL,1);
    check(step.position==0 && close_enough(step.net_factor,.9999),"sale costs one-way fee and removes market exposure");
    step=trading_step(market,22,25,0,HOLD,1);
    check(step.net_factor==1,"flat hold earns zero and pays no fees");
    agent_init(&agent,TRADING_STATES,3,0);
    trading_train(market,&cuts,&agent,1800,2,11,1,NULL);
    Agent before=agent;
    Performance cash=trading_evaluate(market,&cuts,NULL,CASH,2400,3000,1,NULL);
    Performance free_hold=trading_evaluate(market,&cuts,NULL,BUY_HOLD,2400,3000,0,NULL);
    Performance paid_hold=trading_evaluate(market,&cuts,NULL,BUY_HOLD,2400,3000,1,NULL);
    double price_ratio=market->bars[2999].open/market->bars[2400].open;
    check(cash.total_return==0 && cash.orders==0 && isnan(cash.sharpe),"cash is a genuine zero-return baseline");
    check(close_enough(free_hold.total_return,price_ratio-1),"buy-and-hold matches direct price ratio");
    check(close_enough(paid_hold.total_return,price_ratio*.9999*.9999-1) && paid_hold.orders==2,
          "buy-and-hold pays only entry and exit costs");
    trading_evaluate(market,&cuts,&agent,POLICY_Q,2400,3000,1,NULL);
    check(!memcmp(&before,&agent,sizeof(agent)),"held-out evaluation cannot update Q tables");
    agent_init(&loaded,TRADING_STATES,3,0);
    Performance unseen=trading_evaluate(market,&cuts,&loaded,POLICY_Q,2400,3000,1,NULL);
    check(unseen.total_return==0 && unseen.orders==0,"unseen trading states fall back to flat, not arbitrary buy");
    int updates=0;
    for(int i=0;i<TRADING_STATES;i++) updates+=agent.visits[i];
    check(updates==2*(1800-1-WARMUP),"training traverses the full history and records coverage");
    for(int kind=0;kind<3;kind++) {
        FILE *bad=fopen("tests/invalid.tmp","w");assert(bad);
        fprintf(bad,"date,open,close\n");
        for(int t=0;t<150;t++) {
            const char *date=(kind==1 && t==50) ? market->bars[49].date : market->bars[t].date;
            if(kind==2 && t==50) date="2000-02-31";
            double open=(kind==0 && t==50) ? -1 : market->bars[t].open;
            fprintf(bad,"%s,%.12g,%.12g\n",date,open,market->bars[t].close);
        }
        fclose(bad);
        check(!market_load(changed,"tests/invalid.tmp"),kind==0 ? "invalid market prices are rejected" :
              (kind==1 ? "duplicate market dates are rejected" : "invalid calendar dates are rejected"));
    }
    FILE *raw=fopen("tests/intraday.tmp","w");assert(raw);
    fprintf(raw,"DATE_TIME,HIGH,LOW,OPEN,CLOSE\n");
    for(int t=0;t<121;t++) {
        char date[16];strcpy(date,market->bars[t].date);date[4]='.';date[7]='.';
        fprintf(raw,"%s 00:00:00,101,99,100,100\n",date);
    }
    fclose(raw);
    check(market_prepare("tests/intraday.tmp","tests/daily.tmp"),"preparation retains sparse observed days without filling missing bars");
    check(market_load(changed,"tests/daily.tmp") && changed->count==120 &&
          !strcmp(changed->bars[119].date,market->bars[119].date),"preparation excludes only the unfinished final date");
    check(!market_prepare("tests/intraday.tmp","tests/intraday.tmp"),"preparation cannot overwrite its source file");
    remove("tests/intraday.tmp");remove("tests/daily.tmp");
    free(market);free(changed);remove("tests/demo.tmp");remove("tests/invalid.tmp");
    printf("%d checks passed.\n",checks);
    return 0;
}
