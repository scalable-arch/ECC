#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "Config.hh"
#include "Tester.hh"
#include "Scrubber.hh"
#include "FaultDomain.hh"
#include "DomainGroup.hh"
#include "codec.hh"
#include "rs.hh"
#include "progress.hh"
#include "AccessFACH.hh"

//------------------------------------------------------------------------------
char errorName[][10] = {"NE ",
                        "CE ",
                        "DE ",
#ifdef DUE_BREAKDOWN
						"DPa","DNE",
#endif
                        "SDC"
#ifdef DUE_BREAKDOWN
					   ,"SEr"
#endif

						};

//------------------------------------------------------------------------------
void TesterSystem::reset() {
    for (int i=0; i<MAX_YEAR; i++) {
        RetireCntYear[i] = 0l;
        DUECntYear[i] = 0l;
        SDCCntYear[i] = 0l;
#ifdef DUE_BREAKDOWN
        DUE_ParityYear[i] = 0l;
        DUE_NoErasureYear[i] = 0l;
        SDC_ErasureYear[i] = 0l;
#endif
	}
}

//------------------------------------------------------------------------------
void TesterSystem::printSummary(FILE *fd, long runNum) {
    fprintf(fd, "After %ld runs\n", runNum);
    fprintf(fd, "Retire\n");
    for (int yr=1; yr<MAX_YEAR; yr++) {
        fprintf(fd, "%.11f\n", (double)RetireCntYear[yr]/runNum);
    }
    fprintf(fd, "DUE\n");
    for (int yr=1; yr<MAX_YEAR; yr++) {
        fprintf(fd, "%.11f\n", (double)DUECntYear[yr]/runNum);
    }
#ifdef DUE_BREAKDOWN
    fprintf(fd, "DUE_Parity\n");
    for (int yr=1; yr<MAX_YEAR; yr++) {
        fprintf(fd, "%.11f\n", (double)DUE_ParityYear[yr]/runNum);
    }
    fprintf(fd, "DUE_NoErasure\n");
    for (int yr=1; yr<MAX_YEAR; yr++) {
        fprintf(fd, "%.11f\n", (double)DUE_NoErasureYear[yr]/runNum);
    }
#endif
    fprintf(fd, "SDC\n");
    for (int yr=1; yr<MAX_YEAR; yr++) {
        fprintf(fd, "%.11f\n", (double)SDCCntYear[yr]/runNum);
    }
#ifdef DUE_BREAKDOWN
    fprintf(fd, "SDC_Erasure\n");
    for (int yr=1; yr<MAX_YEAR; yr++) {
        fprintf(fd, "%.11f\n", (double)SDC_ErasureYear[yr]/runNum);
    }
#endif
    fflush(fd);
}

//------------------------------------------------------------------------------
double TesterSystem::advance(double faultRate) {
    double result = -log(1.0f - (double) rand() / ((long long) RAND_MAX+1)) / faultRate;
    //printf("- %f\n", result);
    return result;
}

//------------------------------------------------------------------------------
void TesterSystem::test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber, long runCnt, char* filePrefix, int faultCount, std::string *faults) {
    assert(faultCount<=1);  // either no or 1 inherent fault
    Fault *inherentFault = NULL;
    // create log file
    std::string nameBuffer = std::string(filePrefix)+".S";
    if (faultCount==1) {    // no inherent fault
        nameBuffer = nameBuffer+"."+faults[0];
        inherentFault = Fault::genRandomFault(faults[0], NULL);
        dg->setInherentFault(inherentFault);
    }
    FILE *fd = fopen(nameBuffer.c_str(), "w");
    assert(fd!=NULL);

    // reset statistics
    reset();

    // for runCnt times
    for (long runNum=0; runNum<runCnt; runNum++) {
        ProgressStatus(runNum, runCnt);
        if ((runNum==100) || ((runNum!=0)&&(runNum%1000000==0))) {
            showCacheStatus();
            printSummary(fd, runNum);
        }
        if (runNum%10000000==0) {
        //if (runNum%1000000==0) {
            printf("\nProcessing %ldth iteration\n", runNum);
        }

        if (inherentFault!=NULL) {
            dg->setInitialRetiredBlkCount(ecc);
        }

        // inter-arrival time of generating new f
        double hr = 0.;

        while (true) {
            // 1. Advance
            double prevHr = hr;
            hr += advance(dg->getFaultRate());

            if (hr > (MAX_YEAR-1)*24*365) {
                break;
            }

            // 2. scrub soft errors
            scrubber->scrub(dg, hr);

            // 3. generate a fault
            FaultDomain *fd = dg->pickRandomFD();

            // 4. generate an error and decode it
            ErrorType result = fd->genSystemRandomFaultAndTest(ecc);

            // 5. process result
			// default : PF retirement
            if ((result==CE)&&ecc->getDoRetire()&&(fd->getRetiredBlkCount() > ecc->getMaxRetiredBlkCount())) {
//printf("%d %llu %llu\n", ecc->getDoRetire(), fd->getRetiredBlkCount(), ecc->getMaxRetiredBlkCount());
                for (int i=0; i<MAX_YEAR; i++) {
                    if (hr < i*24*365) {
                        RetireCntYear[i]++;
                    }
                }
                break;
            } else if (result==DUE) {
                for (int i=0; i<MAX_YEAR; i++) {
                    if (hr < i*24*365) {
                        DUECntYear[i]++;
                    }
                }
                break;
            } else if (result==SDC) {
// printf("hours %lf (%lfyrs)\n", hr, hr/(24*365));
                for (int i=0; i<MAX_YEAR; i++) {
                    if (hr < i*24*365) {
                        SDCCntYear[i]++;
                    }
                }
                break;
            } 
#ifdef DUE_BREAKDOWN			
			else if (result==DUE_Parity) {
                for (int i=0; i<MAX_YEAR; i++) {
                    if (hr < i*24*365) {
                        DUE_ParityYear[i]++;
                        DUECntYear[i]++;
                    }
                }
                break;
            } else if (result==DUE_NoErasure) {
                for (int i=0; i<MAX_YEAR; i++) {
                    if (hr < i*24*365) {
                        DUE_NoErasureYear[i]++;
                        DUECntYear[i]++;
                    }
                }
                break;
            } else if (result==SDC_Erasure) {
printf("== hours %lf (%lfyrs)\n", hr, hr/(24*365));
                for (int i=0; i<MAX_YEAR; i++) {
                    if (hr < i*24*365) {
                        SDC_ErasureYear[i]++;
                        SDCCntYear[i]++;
                    }
                }
                break;
            }
#endif
        }

        dg->clear();
        ecc->clear();
    }
    printSummary(fd, runCnt);

    fclose(fd);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void TesterScenario::reset() {
    for (int i=0; i<=SDC; i++) {
        errorCnt[i] = 0l;
    }
}
//------------------------------------------------------------------------------
void TesterScenario::printSummary(FILE *fd, long runNum) {
    fprintf(fd, "After %ld runs\n", runNum);
    for (int i=0; i<=SDC; i++) {
        fprintf(fd, "%s\t%.10f\n", errorName[i], (double)errorCnt[i]/runNum);
    }
    fflush(fd);
}
//------------------------------------------------------------------------------
void TesterScenario::test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber, long runCnt, char* filePrefix, int faultCount, std::string *faults) {
    assert(faultCount!=0);
    // create log file
    std::string nameBuffer = std::string(filePrefix)+"."+faults[0];
    for (int i=1; i<faultCount; i++) {
        nameBuffer = nameBuffer+"."+faults[i];
    }
    FILE *fd = fopen(nameBuffer.c_str(), "w");
    assert(fd!=NULL);

    // reset statistics
    reset();

    // for runCnt times
    for (long runNum=0; runNum<runCnt; runNum++) {
        if ((runNum==100) || ((runNum!=0)&&(runNum%100000000==0))) {
            printSummary(fd, runNum);
        }
        if (runNum%10000000==0) {
            printf("Processing %ldth iteration\n", runNum);
        }

        ErrorType result = dg->getFD()->genScenarioRandomFaultAndTest(ecc, faultCount, faults);

        errorCnt[result]++;
    }
    
	printSummary(fd, runCnt);
	
	fclose(fd);
}


//------------------------------------------------------------------------------
