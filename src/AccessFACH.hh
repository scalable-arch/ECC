#include "FaultCache.hh"
#include "Fault.hh"



void showCacheStatus();
int performAccess(ADDR addr);
double multiBitFault(int nun_fault, int num_set);
double genSingleFault(Fault* _fault);
void setCacheParam(int param, int value);
void initCache();
