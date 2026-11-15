#include "trading.h"
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int valid_date(const char *date) {
    if(strlen(date)!=10 || date[4]!='-' || date[7]!='-') return 0;
    for(int i=0;i<10;i++) if(i!=4 && i!=7 && !isdigit((unsigned char)date[i])) return 0;
    int year,month,day;
    if(sscanf(date,"%d-%d-%d",&year,&month,&day)!=3 || year<1 || month<1 || month>12) return 0;
    const int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    int length=days[month-1];
    if(month==2 && (year%400==0 || (year%4==0 && year%100!=0))) length++;
    return day>=1 && day<=length;
}

int market_load(Market *market, const char *path) {
    FILE *file = fopen(path, "r");
    char line[256];
    market->count = 0;
    if (!file) return 0;
    if (!fgets(line, sizeof(line), file)) {fclose(file);return 0;}
    line[strcspn(line,"\r\n")]='\0';
    if(strcmp(line,"date,open,close")) {fclose(file);return 0;}
    while (fgets(line, sizeof(line), file)) {
        Bar bar;
        char extra;
        if (market->count >= MAX_BARS || sscanf(line, "%15[^,],%lf,%lf %c", bar.date, &bar.open, &bar.close, &extra) != 3 ||
            !valid_date(bar.date) ||
            !isfinite(bar.open) || !isfinite(bar.close) || bar.open <= 0 || bar.close <= 0 ||
            (market->count && strcmp(bar.date, market->bars[market->count-1].date) <= 0)) {
            fclose(file); return 0;
        }
        market->bars[market->count++] = bar;
    }
    int ok = !ferror(file) && market->count >= 120;
    fclose(file);
    return ok;
}

/* Aggregate every observed source-clock date. A day is emitted only once the
   next date is observed. Missing intraday bars never decide future eligibility. */
int market_prepare(const char *input, const char *output) {
    if(!strcmp(input,output)) return 0;
    FILE *in=fopen(input,"r");
    if(!in) return 0;
    FILE *out=fopen(output,"w");
    if(!out) {fclose(in);return 0;}
    char line[256],day[11]="",previous[32]="";
    double day_open=0,day_close=0;
    int count=0,kept=0,incomplete=0;
    if(!fgets(line,sizeof(line),in) || strncmp(line,"DATE_TIME,HIGH,LOW,OPEN,CLOSE",29)) {
        fclose(in);fclose(out);return 0;
    }
    fprintf(out,"date,open,close\n");
    while(fgets(line,sizeof(line),in)) {
        char stamp[32];double high,low,open,close;
        int hour,minute,second;
        if(sscanf(line,"%31[^,],%lf,%lf,%lf,%lf",stamp,&high,&low,&open,&close)!=5 ||
           strlen(stamp)!=19 || sscanf(stamp+11,"%d:%d:%d",&hour,&minute,&second)!=3 ||
           hour<0 || hour>23 || minute<0 || minute>59 || minute%15 || second!=0 ||
           !isfinite(high) || !isfinite(low) || !isfinite(open) || !isfinite(close) ||
           low<=0 || high<low || open<low || open>high || close<low || close>high ||
           (previous[0] && strcmp(stamp,previous)<=0)) {
            fclose(in);fclose(out);return 0;
        }
        char current[11];memcpy(current,stamp,10);current[10]='\0';
        char iso[11];strcpy(iso,current);iso[4]='-';iso[7]='-';
        if(stamp[4]!='.' || stamp[7]!='.' || stamp[10]!=' ' || !valid_date(iso)) {fclose(in);fclose(out);return 0;}
        if(strcmp(current,day)) {
            if(count) {
                fprintf(out,"%.4s-%.2s-%.2s,%.10g,%.10g\n",day,day+5,day+8,day_open,day_close);
                kept++;if(count!=96) incomplete++;
            }
            strcpy(day,current);count=0;
        }
        if(!count) day_open=open;
        day_close=close;count++;strcpy(previous,stamp);
    }
    /* Do not pretend the final observed date is a completed daily session. */
    int ok=!ferror(in) && !ferror(out) && kept>=120;
    fclose(in);fclose(out);
    printf("Prepared %d completed source-clock dates; retained %d dates with missing intraday bars.\n",kept,incomplete);
    printf("Excluded final observed date %s because completion is not established.\n",day);
    return ok;
}

static void next_day(int *year, int *month, int *day) {
    const int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    int length=days[*month-1];
    if (*month==2 && (*year%400==0 || (*year%4==0 && *year%100!=0))) length++;
    (*day)++;
    if (*day>length) { *day=1;(*month)++; }
    if (*month>12) { *month=1;(*year)++; }
}

int market_demo(const char *path, unsigned int seed) {
    FILE *file=fopen(path,"w");
    if (!file) return 0;
    fprintf(file,"date,open,close\n");
    double price=100, last_return=0;
    int year=2000,month=1,day=3,weekday=0;
    for(int t=0;t<3000;t++) {
        double volatility=(t/200)%2 ? .012 : .006;
        double gap=(random_number(&seed)-.5)*.003;
        double open=price*exp(gap);
        double noise=0;
        for(int j=0;j<12;j++) noise+=random_number(&seed);
        double r=.0001+.15*last_return+volatility*(noise-6);
        price=open*exp(r);last_return=r;
        fprintf(file,"%04d-%02d-%02d,%.10f,%.10f\n",year,month,day,open,price);
        do { next_day(&year,&month,&day);weekday=(weekday+1)%7; } while(weekday>=5);
    }
    return fclose(file)==0;
}

Features market_features(const Market *market,int k) {
    Features f;
    f.daily_return=log(market->bars[k].close/market->bars[k-1].close);
    f.momentum=log(market->bars[k].close/market->bars[k-5].close);
    double sum=0;
    for(int j=k-19;j<=k;j++) {
        double r=log(market->bars[j].close/market->bars[j-1].close);
        sum+=r*r;
    }
    f.volatility=sqrt(sum/20);
    return f;
}

static int compare_double(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;
    return (x>y)-(x<y);
}

Cuts market_fit_cuts(const Market *market,int end) {
    double returns[MAX_BARS],momentum[MAX_BARS],volatility[MAX_BARS];
    int n=0;
    for(int k=20;k<end;k++) {
        Features f=market_features(market,k);
        returns[n]=f.daily_return;momentum[n]=f.momentum;volatility[n]=f.volatility;n++;
    }
    qsort(returns,(size_t)n,sizeof(double),compare_double);
    qsort(momentum,(size_t)n,sizeof(double),compare_double);
    qsort(volatility,(size_t)n,sizeof(double),compare_double);
    Cuts cuts={returns[n/3],returns[2*n/3],momentum[n/3],momentum[2*n/3],volatility[n/2]};
    return cuts;
}

int trading_state(const Market *market,const Cuts *cuts,int time,int position) {
    Features f=market_features(market,time-1);
    int r=f.daily_return<cuts->return_low ? 0 : (f.daily_return<cuts->return_high ? 1 : 2);
    int m=f.momentum<cuts->momentum_low ? 0 : (f.momentum<cuts->momentum_high ? 1 : 2);
    int v=f.volatility<cuts->volatility_mid ? 0 : 1;
    return ((r*3+m)*2+v)*2+position;
}
