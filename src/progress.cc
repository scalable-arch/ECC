#include "progress.hh"

void ProgressStatus(unsigned long long count, unsigned long long max_count){
    float bar_tick = (float)100/LEN;
    printf("\r%lld/%lld [", count, max_count);
    float percent = (float)count/max_count * 100;
    int bar_count = percent/bar_tick;

    for(int i=0; i<LEN; i++){
        if(bar_count > i){
            printf("%c", bar);
        }else{
            printf("%c", blank);
        }
    }
    printf("] %0.2f%%", percent);
}