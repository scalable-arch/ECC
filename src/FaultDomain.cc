#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "Config.hh"
#include "FaultDomain.hh"
#include "ECC.hh"

//------------------------------------------------------------------------------
ErrorType worseErrorType(ErrorType a, ErrorType b) {
    return (a>b) ? a : b;
}

//------------------------------------------------------------------------------
ErrorType FaultDomain::genScenarioRandomFaultAndTest(ECC *ecc, int faultCount, std::string *faults) {
    CacheLine blk = {pinsPerDevice, (devicesPerRank -(int) retiredChipIDList.size()) * pinsPerDevice - (int) retiredPinIDList.size(), blkHeight};

	clear();

    int fault1ChipID = -1;
    int fault2ChipID = -1;
    int fault3ChipID = -1;

    // generate fault1
    if (faultCount > 0) {
        Fault *fault1 = Fault::genRandomFault(faults[0], this);
		//GONG
		operationalFaultList.push_back(fault1);
        
		fault1->genRandomError(&blk);
        fault1ChipID = fault1->getChipID();
//        delete fault1;
    }
    // generate fault2
    if (faultCount > 1) {
        Fault *fault2 = NULL;
        do {
            if (fault2!=NULL) delete fault2;  // delete previous one
            fault2 = Fault::genRandomFault(faults[1], this);
        } while (fault2->getChipID()==fault1ChipID);
		//GONG
		operationalFaultList.push_back(fault2);
        
		fault2->genRandomError(&blk);
        fault2ChipID = fault2->getChipID();
//        delete fault2;
    }

    // generate fault3
    if (faultCount > 2) {
        Fault *fault3 = NULL;
        do {
            if (fault3!=NULL) delete fault3;  // delete previous one
            fault3 = Fault::genRandomFault(faults[2], this);
        } while ((fault3->getChipID()==fault1ChipID)||(fault3->getChipID()==fault2ChipID));
		//GONG
		operationalFaultList.push_back(fault3);
        
		fault3->genRandomError(&blk);
        fault3ChipID = fault3->getChipID();
//        delete fault3;
    }

    // generate fault4
    if (faultCount > 3) {
        Fault *fault4 = NULL;
        do {
            if (fault4!=NULL) delete fault4;  // delete previous one
            fault4 = Fault::genRandomFault(faults[3], this);
        } while ((fault4->getChipID()==fault1ChipID)||(fault4->getChipID()==fault2ChipID)||(fault4->getChipID()==fault3ChipID));
		//GONG
		operationalFaultList.push_back(fault4);
        
		fault4->genRandomError(&blk);
//        delete fault4;
    }
    assert(faultCount<=4);

    // decode and report the result
    ErrorType result = ecc->decode(this, blk);

    return result;
}

ErrorType FaultDomain::genSystemRandomFaultAndTest(ECC *ecc) {
    CacheLine blk = {pinsPerDevice, (devicesPerRank -(int) retiredChipIDList.size()) * pinsPerDevice - (int) retiredPinIDList.size(), blkHeight};
    Fault *newFault;
    ErrorType result = NE;

    //----------------------------------------------------------
    // 1. generate a new fault
    //----------------------------------------------------------
    std::string newFaultType = faultRateInfo->pickRandomType();
    newFault = Fault::genRandomFault(newFaultType, this);

    //----------------------------------------------------------
    // check whether the fault is on a retired chip or pin
    //----------------------------------------------------------
    // a new fault on already retired chip -> skip
    for (auto it = retiredChipIDList.cbegin(); it != retiredChipIDList.cend(); ++it) {
        if (newFault->getChipID()==(*it)) {
            delete newFault;
            return NE;
        }
    }
    // a new pin fault on already retired pin -> skip
    if (newFault->getIsSingleDQ()) {
        // retired pin
        for (auto it = retiredPinIDList.cbegin(); it != retiredPinIDList.cend(); ++it) {
            if (newFault->getPinID()==(*it)) {
                delete newFault;
                return NE;
            }
        }
    }

#if 1
    operationalFaultList.push_back(newFault);

    //----------------------------------------------------------
    // 3. check overlapping previous fault
    // - check the most severe one
    //----------------------------------------------------------
    auto it1 = operationalFaultList.crbegin();
    bool overlap1 = false;
    for (++it1; it1!=operationalFaultList.crend(); ++it1) {
        if ((*it1)->overlap(newFault)) {
            overlap1 = true;
            bool overlap2 = false;
            auto it2 = it1;
            for (++it2; it2!=operationalFaultList.crend(); ++it2) {
                if ((*it2)->overlap(newFault) && (*it2)->overlap(*it1)) {
                    overlap2 = true;
                    bool overlap3 = false;
                    auto it3 = it2;
                    for (++it3; it3!=operationalFaultList.crend(); ++it3) {
                        if ((*it3)->overlap(newFault) && (*it3)->overlap(*it1) && (*it3)->overlap(*it2)) {
                            overlap3 = true;
                            bool overlap4 = false;
                            auto it4 = it3;
                            for (++it4; it4!=operationalFaultList.crend(); ++it4) {
                                if ((*it4)->overlap(newFault) && (*it4)->overlap(*it1) && (*it4)->overlap(*it2) && (*it4)->overlap(*it3)) {
                                    overlap4 = true;
                                    assert(0);
                                }
                            }
                            if (!overlap4) {
                                blk.reset();
                                if (inherentFault!=NULL) inherentFault->genRandomError(&blk);
                                (*it1)->genRandomError(&blk);
                                (*it2)->genRandomError(&blk);
                                (*it3)->genRandomError(&blk);
                                newFault->genRandomError(&blk);
                                result = worseErrorType(result, ecc->decode(this, blk));
                            }
                        }
                    }
                    if (!overlap3) {
                        blk.reset();
                        if (inherentFault!=NULL) inherentFault->genRandomError(&blk);
                        (*it1)->genRandomError(&blk);
                        (*it2)->genRandomError(&blk);
                        newFault->genRandomError(&blk);
                        result = worseErrorType(result, ecc->decode(this, blk));
                    }
                }
            }
            if (!overlap2) {
                blk.reset();
                if (inherentFault!=NULL) inherentFault->genRandomError(&blk);
                (*it1)->genRandomError(&blk);
                newFault->genRandomError(&blk);
                result = worseErrorType(result, ecc->decode(this, blk));
            }
        }
    }
    if (!overlap1) {
        blk.reset();
        if (inherentFault!=NULL) inherentFault->genRandomError(&blk);
        newFault->genRandomError(&blk);
        result = worseErrorType(result, ecc->decode(this, blk));
    }

    if ((result==CE)&&ecc->getDoRetire()&&ecc->needRetire(this, newFault)) {
        retiredBlkCount += newFault->getAffectedBlkCount();
        operationalFaultList.remove(newFault);
        delete newFault;
    }
    return result;
#else
    auto faultEnd = operationalFaultList.rend();
    for (auto it1 = operationalFaultList.rbegin(); it1!=faultEnd; ++it1) {
        Fault *fault1 = *it1;

        fault1->genRandomError(&blk);
        result = ecc->decode(this, blk);
        if (result==SDC) return SDC;

        auto it2 = it1;
        for (++it2; it2!=faultEnd; ++it2) {
            Fault *fault2 = (*it2);

            if (fault2->overlap(fault1)) {
                fault2->genRandomError(&blk);
                result = worseErrorType(result, ecc->decode(this, blk));
                if (result==SDC) return SDC;

                auto it3 = it2;
                for (++it3; it3!=faultEnd; ++it3) {
                    Fault *fault3 = (*it3);
                    if (fault3->overlap(fault1) && fault3->overlap(fault2)) {
                        fault3->genRandomError(&blk);
                        result = worseErrorType(result, ecc->decode(this, blk));
                        if (result==SDC) return SDC;

                        auto it4 = it3;
                        for (++it4; it4!=faultEnd; ++it4) {
                            Fault *fault4 = (*it4);
                            if (fault4->overlap(fault1) && fault4->overlap(fault2) && fault4->overlap(fault3)) {
                                fault4->genRandomError(&blk);
                                result = worseErrorType(result, ecc->decode(this, blk));
                                if (result==SDC) return SDC;
                            }
                        }
                    }
                }

            }
        }
    }

    return result;
#endif
}

//------------------------------------------------------------------------------
void FaultDomain::setInitialRetiredBlkCount(ECC *ecc) {
    retiredBlkCount = ecc->getInitialRetiredBlkCount(this, inherentFault);
    //if (retiredBlkCount!=0) {
    //    printf("%lld\n", retiredBlkCount);
    //}
}

void FaultDomain::scrub() {
    auto it = operationalFaultList.begin();
    while (it != operationalFaultList.end()) {
        if ((*it)->getIsTransient()==true) {
            delete *it;
            it = operationalFaultList.erase(it);
            // continue without advancing the iterator
        } else {
            ++it;
        }
    }
}

//------------------------------------------------------------------------------
void FaultDomain::retirePin(int pinID) {
    auto it = operationalFaultList.begin();
    while (it != operationalFaultList.end()) {
        if ((*it)->getIsSingleDQ() && ((*it)->getPinID()==pinID)) {
            // pin fault
            delete *it;
            it = operationalFaultList.erase(it);
            // continue without advancing the iterator
        } else {
            ++it;
        }
    }
    retiredPinIDList.push_back(pinID);
}

void FaultDomain::retireChip(int chipID) {
    auto it = operationalFaultList.begin();
    while (it != operationalFaultList.end()) {
        if ((*it)->getChipID()==chipID) {
            delete *it;
            it = operationalFaultList.erase(it);
            // continue without advancing the iterator
        } else {
            ++it;
        }
    }
    retiredChipIDList.push_back(chipID);
}

void FaultDomain::clear() {
    for (auto it = operationalFaultList.begin(); it != operationalFaultList.end(); ++it) {
        delete *it;
    }
    operationalFaultList.clear();
    retiredChipIDList.clear();
    retiredPinIDList.clear();

}

void FaultDomain::print(FILE *fd) const {
    for (auto it = operationalFaultList.begin(); it != operationalFaultList.end(); it++) {
        (*it)->print(fd);
    }
}

//------------------------------------------------------------------------------


//GONG
int FaultDomain::FaultyChipDetect(){
	int num_chip_fault=0;
	Fault *cur_fault;
	Fault *prev_fault=NULL;
	std::set<int> chip_set;//set of faulty chips 
	std::pair<std::set<int>::iterator, bool> ret;
	//if only one fault exists
	if(operationalFaultList.size()==1){
		chip_set.insert((*operationalFaultList.begin())->getChipID());
		num_chip_fault=1;
	}else{
		for(std::list<Fault*>::reverse_iterator it = operationalFaultList.rbegin(); it != operationalFaultList.rend(); it++){
			//check if it has chip-level fault (bank, rank)
			//ideally we can figure out faulty chip by accessing/correcting adjacent blocks
			if( !(*it)->getIsSingleBeat() ){
				cur_fault = *it;
				if(cur_fault->overlap(prev_fault)){ 
					ret = chip_set.insert(cur_fault->getChipID());
					if(ret.second) num_chip_fault++;
				}
				//(*it)->print();
			}
			prev_fault = *it;
		}
	}
	if(num_chip_fault==1 ) return *chip_set.begin();
	else return -1;
		
}
