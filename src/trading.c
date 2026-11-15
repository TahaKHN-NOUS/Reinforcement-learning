#include "trading.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TradeStep trading_step(const Market *market,int t,int end,int position,int action,double cost_bps) {
    assert(t>=WARMUP && t+1<end && end<=market->count);
    assert(position==0 || position==1);
    assert(action>=BUY && action<=HOLD);
    assert(cost_bps>=0 && cost_bps<10000);
    TradeStep out;
    out.position=action==BUY ? 1 : (action==SELL ? 0 : position);
    out.done=t+1==end-1;
    double cost=cost_bps/10000;
    out.trade_cost_fraction=abs(out.position-position)*cost;
    out.gross_factor=1+out.position*(market->bars[t+1].open/market->bars[t].open-1);
    out.liquidation_fraction=out.done ? out.position*cost : 0;
    out.net_factor=(1-out.trade_cost_fraction)*out.gross_factor*(1-out.liquidation_fraction);
    out.next_position=out.done ? 0 : out.position;
    return out;
}

void trading_train(const Market *market,const Cuts *cuts,Agent *agent,int end,
                   int episodes,unsigned int seed,double cost_bps,FILE *curve) {
    for(int ep=0;ep<episodes;ep++) {
        int position=0;
        double epsilon=1-.95*ep/(episodes>1 ? episodes-1 : 1);
        double growth=0;
        for(int t=WARMUP;t<end-1;t++) {
            int state=trading_state(market,cuts,t,position);
            int action=agent_action(agent,state,epsilon,&seed);
            TradeStep out=trading_step(market,t,end,position,action,cost_bps);
            double reward=100*log(out.net_factor);
            int next=out.done ? 0 : trading_state(market,cuts,t+1,out.next_position);
            agent_update(agent,state,action,reward,next,out.done,.1,.95,&seed);
            position=out.next_position;growth+=log(out.net_factor);
        }
        if(curve && (ep==0 || (ep+1)%20==0)) fprintf(curve,"%d,%.10g\n",ep+1,expm1(growth));
    }
}

Performance trading_evaluate(const Market *market,const Cuts *cuts,const Agent *agent,
                             int policy,int start,int end,double cost_bps,FILE *ledger) {
    Performance p={0};
    int position=0;
    double nav=10000,gross=1,peak=10000,sum=0,sum2=0;
    if(ledger) fprintf(ledger,"date,next_date,state,action,position,price_return,trade_fee,liquidation_fee,net_return,nav\n");
    for(int t=start;t<end-1;t++) {
        int state=trading_state(market,cuts,t,position);
        int action;
        if(policy==CASH) action=SELL;
        else if(policy==BUY_HOLD) action=BUY;
        else if(policy==MOMENTUM) action=market_features(market,t-1).momentum>0 ? BUY : SELL;
        else action=agent->visits[state] ? agent_greedy(agent,state) : SELL;
        TradeStep out=trading_step(market,t,end,position,action,cost_bps);
        double trade_fee=nav*out.trade_cost_fraction;
        double before_exit=(nav-trade_fee)*out.gross_factor;
        double exit_fee=before_exit*out.liquidation_fraction;
        nav=before_exit-exit_fee;
        p.fees+=trade_fee+exit_fee;
        p.orders+=abs(out.position-position)+(out.done ? out.position : 0);
        gross*=out.gross_factor;
        if(nav>peak) peak=nav;
        if(nav/peak-1<p.drawdown) p.drawdown=nav/peak-1;
        double r=out.net_factor-1;
        sum+=r;sum2+=r*r;p.periods++;
        p.log_growth+=log(out.net_factor);
        if(ledger) fprintf(ledger,"%s,%s,%d,%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g\n",
            market->bars[t].date,market->bars[t+1].date,state,action,out.position,
            market->bars[t+1].open/market->bars[t].open-1,trade_fee,exit_fee,r,nav);
        position=out.next_position;
    }
    p.total_return=nav/10000-1;p.gross_return=gross-1;
    double variance=p.periods>1 ? (sum2-sum*sum/p.periods)/(p.periods-1) : 0;
    p.sharpe=variance>1e-20 ? sqrt(252)*sum/p.periods/sqrt(variance) : NAN;
    return p;
}

static void write_score(FILE *file,const char *phase,const char *name,int seed,double cost,Performance p) {
    fprintf(file,"%s,%s,%d,%.3f,%d,%d,%.12g,%.12g,%.12g,%.12g,%.12g\n",
        phase,name,seed,cost,p.periods,p.orders,p.total_return,p.gross_return,p.sharpe,p.drawdown,p.fees);
}

int trading_experiment(const char *path,const char *output,int seeds) {
    Market *market=malloc(sizeof(Market));
    Agent *agents=calloc((size_t)(2*seeds),sizeof(Agent));
    FILE *scores=NULL,*file=NULL;
    int ok=0;
    char filename[512];
    const char *names[5]={"q_learning","double_q","cash","buy_hold","momentum"};
    if(!market || !agents || !market_load(market,path)) { fprintf(stderr,"Cannot load daily market CSV: %s\n",path);goto cleanup; }
    int train_end=(int)(market->count*.6),validation_end=(int)(market->count*.8);
    Cuts cuts=market_fit_cuts(market,train_end);
    snprintf(filename,sizeof(filename),"%s/trading_scores.csv",output);
    scores=fopen(filename,"w");if(!scores) goto cleanup;
    fprintf(scores,"phase,policy,seed,one_way_cost_bps,periods,orders,net_return,gross_return,session_sharpe_252,max_drawdown,fees_on_10000\n");
    double validation[2]={0,0};
    for(int method=0;method<2;method++) {
        for(int k=0;k<seeds;k++) {
            Agent *agent=&agents[method*seeds+k];
            agent_init(agent,TRADING_STATES,3,method);
            FILE *curve=NULL;
            if(k==0) {
                snprintf(filename,sizeof(filename),"%s/trading_%s_curve.csv",output,names[method]);
                curve=fopen(filename,"w");if(!curve) goto cleanup;
                fprintf(curve,"episode,exploratory_training_net_return\n");
            }
            trading_train(market,&cuts,agent,train_end,400,(unsigned int)(11*(k+1)),1.0,curve);
            if(curve) fclose(curve);
            Performance p=trading_evaluate(market,&cuts,agent,method,train_end,validation_end,1.0,NULL);
            validation[method]+=p.log_growth/seeds;
            write_score(scores,"validation",names[method],11*(k+1),1.0,p);
            if(k==0) {
                snprintf(filename,sizeof(filename),"%s/trading_%s_policy.txt",output,names[method]);
                if(!agent_save(agent,filename)) goto cleanup;
            }
        }
    }
    /* Choose between the two learned methods and cash before evaluating test data.
       Average log wealth across all fixed training seeds; never pick a test winner. */
    int selected=CASH;double best=0;
    for(int method=0;method<2;method++) if(validation[method]>best) {best=validation[method];selected=method;}
    printf("Validation-selected policy: %s (cash is an eligible fallback).\n",names[selected]);
    const double costs[5]={0,.5,1,2,5};
    for(int policy=0;policy<5;policy++) {
        int runs=policy<2 ? seeds : 1;
        for(int k=0;k<runs;k++) for(int c=0;c<5;c++) {
            Agent *agent=policy<2 ? &agents[policy*seeds+k] : NULL;
            FILE *ledger=NULL;
            if(k==0 && costs[c]==1) {
                snprintf(filename,sizeof(filename),"%s/trading_%s_ledger.csv",output,names[policy]);
                ledger=fopen(filename,"w");if(!ledger) goto cleanup;
            }
            Performance p=trading_evaluate(market,&cuts,agent,policy,validation_end,market->count,costs[c],ledger);
            if(ledger) fclose(ledger);
            write_score(scores,"test",names[policy],policy<2 ? 11*(k+1) : 0,costs[c],p);
        }
    }
    snprintf(filename,sizeof(filename),"%s/trading_selected_ledger.csv",output);
    file=fopen(filename,"w");if(!file) goto cleanup;
    trading_evaluate(market,&cuts,selected==CASH ? NULL : &agents[selected*seeds],
                     selected,validation_end,market->count,1.0,file);
    fclose(file);file=NULL;
    snprintf(filename,sizeof(filename),"%s/trading_state_visits.csv",output);
    file=fopen(filename,"w");if(!file) goto cleanup;
    fprintf(file,"state,q_learning_seed11_visits,double_q_seed11_visits\n");
    for(int s=0;s<TRADING_STATES;s++) fprintf(file,"%d,%d,%d\n",s,agents[0].visits[s],agents[seeds].visits[s]);
    fclose(file);file=NULL;
    snprintf(filename,sizeof(filename),"%s/trading_config.txt",output);
    file=fopen(filename,"w");if(!file) goto cleanup;
    fprintf(file,"data=%s\nbars=%d\ntrain_end_exclusive=%d\nvalidation_end_exclusive=%d\n",
        path,market->count,train_end,validation_end);
    fprintf(file,"train_first=%s\ntrain_last=%s\nvalidation_first=%s\nvalidation_last=%s\ntest_first=%s\ntest_last=%s\n",
        market->bars[WARMUP].date,market->bars[train_end-1].date,market->bars[train_end].date,
        market->bars[validation_end-1].date,market->bars[validation_end].date,market->bars[market->count-1].date);
    fprintf(file,"episodes=400\nalpha=0.1\ngamma=0.95\nepsilon_start=1\nepsilon_end=0.05\none_way_cost_bps=1\nseeds=%d\nseed_sequence=11,22,...\n",seeds);
    fprintf(file,"return_low=%.17g\nreturn_high=%.17g\nmomentum_low=%.17g\nmomentum_high=%.17g\nvolatility_mid=%.17g\n",
        cuts.return_low,cuts.return_high,cuts.momentum_low,cuts.momentum_high,cuts.volatility_mid);
    fprintf(file,"validation_q_mean_log_growth=%.17g\nvalidation_double_q_mean_log_growth=%.17g\nselected_policy=%s\nselected_example_seed=%d\n",
        validation[0],validation[1],names[selected],selected==CASH ? 0 : 11);
    ok=1;
cleanup:
    if(file) fclose(file);
    if(scores) fclose(scores);
    free(agents);free(market);
    return ok;
}

int trading_signal(const char *path,const char *directory,int position) {
    Market *market=malloc(sizeof(Market));
    Agent *agent=malloc(sizeof(Agent));
    Cuts cuts={0};
    int found=0,ok=0;
    char filename[512],line[256],selected[32]="";
    snprintf(filename,sizeof(filename),"%s/trading_config.txt",directory);
    FILE *file=fopen(filename,"r");
    if(!file || !market || !agent || !market_load(market,path)) goto cleanup;
    while(fgets(line,sizeof(line),file)) {
        if(sscanf(line,"return_low=%lf",&cuts.return_low)==1) found++;
        if(sscanf(line,"return_high=%lf",&cuts.return_high)==1) found++;
        if(sscanf(line,"momentum_low=%lf",&cuts.momentum_low)==1) found++;
        if(sscanf(line,"momentum_high=%lf",&cuts.momentum_high)==1) found++;
        if(sscanf(line,"volatility_mid=%lf",&cuts.volatility_mid)==1) found++;
        if(sscanf(line,"selected_policy=%31s",selected)==1) found++;
    }
    if(found!=6 || !isfinite(cuts.return_low) || !isfinite(cuts.return_high) ||
       !isfinite(cuts.momentum_low) || !isfinite(cuts.momentum_high) || !isfinite(cuts.volatility_mid) ||
       cuts.return_low>cuts.return_high || cuts.momentum_low>cuts.momentum_high || cuts.volatility_mid<0) goto cleanup;
    int action=SELL,state=trading_state(market,&cuts,market->count,position),unseen=0;
    if(strcmp(selected,"cash")) {
        if(strcmp(selected,"q_learning") && strcmp(selected,"double_q")) goto cleanup;
        snprintf(filename,sizeof(filename),"%s/trading_%s_policy.txt",directory,selected);
        if(!agent_load(agent,filename) || agent->states!=TRADING_STATES || agent->actions!=3 ||
           agent->double_q!=(strcmp(selected,"double_q")==0)) goto cleanup;
        unseen=agent->visits[state]==0;
        action=unseen ? SELL : agent_greedy(agent,state);
    }
    const char *actions[3]={"BUY / target long","SELL / target flat","HOLD current exposure"};
    printf("After close %s: selected=%s state=%d position=%d\n",market->bars[market->count-1].date,selected,state,position);
    printf("Next available open: %s%s\n",actions[action],unseen ? " (unseen training state: flat fallback)" : "");
    printf("This is a policy action, not a price forecast or a calibrated probability.\n");
    ok=1;
cleanup:
    if(file) fclose(file);
    free(market);free(agent);
    if(!ok) fprintf(stderr,"Cannot load signal input or saved model.\n");
    return ok;
}
