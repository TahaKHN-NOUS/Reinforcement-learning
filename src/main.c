#include "maze.h"
#include "trading.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv) {
    if(argc==5 && !strcmp(argv[1],"signal")) {
        if(strcmp(argv[4],"0") && strcmp(argv[4],"1")) return 1;
        return trading_signal(argv[2],argv[3],atoi(argv[4])) ? 0 : 1;
    }
    if(argc==4 && !strcmp(argv[1],"prepare")) return market_prepare(argv[2],argv[3]) ? 0 : 1;
    if(argc==3 && !strcmp(argv[1],"demo")) return market_demo(argv[2],12345) ? 0 : 1;
    if((argc==4 || argc==5) && (!strcmp(argv[1],"maze") || !strcmp(argv[1],"trade"))) {
        int seeds=argc==5 ? atoi(argv[4]) : 10;
        if(seeds<1 || seeds>20) {fprintf(stderr,"Seed count must be 1..20.\n");return 1;}
        int ok=!strcmp(argv[1],"maze") ? maze_experiment(argv[2],argv[3],seeds) : trading_experiment(argv[2],argv[3],seeds);
        if(!ok) fprintf(stderr,"Experiment failed. Check input format and create the output directory first.\n");
        return ok ? 0 : 1;
    }
    fprintf(stderr,"Usage:\n  %s maze maze.txt output_directory [seed_count]\n  %s trade prices.csv output_directory [seed_count]\n  %s demo demo.csv\n  %s prepare EURUSD-2000-2020-15m.csv daily.csv\n  %s signal prices.csv model_directory position_0_or_1\n",argv[0],argv[0],argv[0],argv[0],argv[0]);
    return 1;
}
