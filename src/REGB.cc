#include "REGB.hh"
#include "FaultDomain.hh"
#include "hsiao.hh"

//------------------------------------------------------------------------------
OnChip64b::OnChip64b() {
    onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
}

ErrorType OnChip64b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    // on-chip ECC
    ECCWord msg = {72, 64};
    ECCWord decoded = {72, 64};

    if (errorBlk.isZero()) {
        return NE;
    }
    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
            if (result==CE) {
                for (int j=0; j<72; j++) {
                    if (msg.getBit(j)!=decoded.getBit(j)) {
                        errorBlk.invBit(i*4 + j%4 + (j/4)*errorBlk.getChannelWidth());
                    }
                }
            }
        }
    }

    if (errorBlk.isZero()) {
        return CE;
    } else {
        return SDC;
    }
}

unsigned long long OnChip64b::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
        assert(eccBlkSize==72);
        double goodECCBlkProb = pow(1-cellFaultRate, eccBlkSize) + eccBlkSize*pow(1-cellFaultRate, eccBlkSize-1) * cellFaultRate;
        double goodBlkProb = pow(goodECCBlkProb, fd->getChannelWidth()/fd->getChipWidth());
        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/128;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

//------------------------------------------------------------------------------
OnChip72bSECDED::OnChip72bSECDED() : SECDED72b() {
    onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
}

ErrorType OnChip72bSECDED::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    // on-chip ECC
    ECCWord msg = {72, 64};
    ECCWord decoded = {72, 64};

    if (errorBlk.isZero()) {
        return NE;
    }
    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
            if (correctedPosSet.size()==1) {
                auto it = correctedPosSet.begin();
                errorBlk.invBit(i*4 + (*it)%4 + ((*it)/4)*errorBlk.getChannelWidth());
                correctedPosSet.clear();
            }
        }
    }

    if (errorBlk.isZero()) {
        return CE;
    } else {
        return SECDED72b::decodeInternal(fd, errorBlk);
    }
}

unsigned long long OnChip72bSECDED::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
        assert(eccBlkSize==72);
        double goodECCBlkProb = pow(1-cellFaultRate, eccBlkSize) + eccBlkSize*pow(1-cellFaultRate, eccBlkSize-1) * cellFaultRate;
        double goodBlkProb = pow(goodECCBlkProb, fd->getChannelWidth()/fd->getChipWidth());
        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/128;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

//------------------------------------------------------------------------------
OnChip72bAMD::OnChip72bAMD(bool _doPostprocess)
: AMDChipkill72b(_doPostprocess) {
    onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
}

ErrorType OnChip72bAMD::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    // on-chip ECC
    ECCWord msg = {72, 64};
    ECCWord decoded = {72, 64};

    if (errorBlk.isZero()) {
        return NE;
    }
    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
            if (correctedPosSet.size()==1) {
                auto it = correctedPosSet.begin();
                errorBlk.invBit(i*4 + (*it)%4 + ((*it)/4)*errorBlk.getChannelWidth());
                correctedPosSet.clear();
            }
        }
    }

    if (errorBlk.isZero()) {
        return CE;
    } else {
        correctedPosSet.clear();
        ErrorType result = AMDChipkill72b::decodeInternal(fd, errorBlk);
        result = AMDChipkill72b::postprocess(fd, result);
        return result;
    }
}

unsigned long long OnChip72bAMD::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
        assert(eccBlkSize==72);
        double goodECCBlkProb = pow(1-cellFaultRate, eccBlkSize) + eccBlkSize*pow(1-cellFaultRate, eccBlkSize-1) * cellFaultRate;
        double goodBlkProb = pow(goodECCBlkProb, fd->getChannelWidth()/fd->getChipWidth());
        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/128;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

//------------------------------------------------------------------------------
OnChip72bQPC72b::OnChip72bQPC72b(int correction, int maxChips)
    : QPC72b(correction, maxChips) {
    onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
}

ErrorType OnChip72bQPC72b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    // on-chip ECC
    ECCWord msg = {72, 64};
    ECCWord decoded = {72, 64};

    if (errorBlk.isZero()) {
        return NE;
    }
    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
            if (correctedPosSet.size()==1) {
                auto it = correctedPosSet.begin();
                errorBlk.invBit(i*4 + (*it)%4 + ((*it)/4)*errorBlk.getChannelWidth());
                correctedPosSet.clear();
            }
        }
    }

    if (errorBlk.isZero()) {
        return CE;
    } else {
        ErrorType result = QPC72b::decodeInternal(fd, errorBlk);
        return QPC72b::postprocess(fd, result);
    }
}

unsigned long long OnChip72bQPC72b::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
        assert(eccBlkSize==72);
        double goodECCBlkProb = pow(1-cellFaultRate, eccBlkSize) + eccBlkSize*pow(1-cellFaultRate, eccBlkSize-1) * cellFaultRate;
        double goodBlkProb = pow(goodECCBlkProb, fd->getChipCount());
        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/128;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

//------------------------------------------------------------------------------
QPC72bREGB::QPC72bREGB(bool _doFaultDiagnosis, bool _doRetire)
    : QPC72b(4, 3, false), doFaultDiagnosis(_doFaultDiagnosis) {
    secondCodec = new RS<2, 8>("DPC\t18\t4\t", 72, 4, 2);
    thirdCodec = new RS<2, 8>("TPC\t18\t4\t", 72, 7, 2);
    fourthCodec = new RS<2, 8>("TPC\t18\t4\t", 72, 6, 2);
    doRetire = _doRetire;
}

ErrorType QPC72bREGB::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    CacheLine temp(errorBlk.getChipWidth(), errorBlk.getChannelWidth(), errorBlk.getBeatHeight());
    temp.clone(&errorBlk);
    ErrorType result;
    result = QPC72b::decodeInternal(fd, errorBlk);
    result = QPC72b::postprocess(fd, result);

    if ((result==DUE) && doFaultDiagnosis) {
        assert(correctedPosSet.size()==0);
        Fault *newFault = fd->operationalFaultList.back();

        std::list<Fault *> overlapManyDQFaults;
        std::list<Fault *> overlapFewDQFaults;
        // fault diagnosis
        for (auto it = fd->operationalFaultList.cbegin(); it != fd->operationalFaultList.cend(); it++) {
            if ((*it)->overlap(newFault)) { // can be the new fault itself
                if ((*it)->getIsTransient() && !(*it)->getIsMultiColumn() && !(*it)->getIsMultiRow()) {
                    // transient single-bit/word fault
                    // -> cannot diagnoze
                } else {
                    if ((*it)->getNumDQ()>2) {
                        overlapManyDQFaults.push_back(*it);
                    } else {
                        overlapFewDQFaults.push_back(*it);
                    }
                }
            }
        }
        if (overlapManyDQFaults.size()>0) {
            int faultPos = rand()%overlapManyDQFaults.size();
            int pos;
            for (auto it = overlapManyDQFaults.cbegin(); it != overlapManyDQFaults.cend();) {
                if (pos==faultPos) {
                    int chipID = (*it)->getChipID();
                    for (int i=0; i<errorBlk.getChipWidth(); i++) {
                        for (int j=0; j<errorBlk.getBeatHeight(); j++) {
                            errorBlk.setBit(j*errorBlk.getChannelWidth()+chipID*4+i, 0);
                        }
                        correctedPosSet.insert(chipID*errorBlk.getChipWidth()+i);
                    }
                    // error correction
                    ECCWord msg = {72*8, 64*8};
                    ECCWord decoded = {72*8, 64*8};

                    msg.extract(&errorBlk, PIN, 0, errorBlk.getChannelWidth());
                    assert(!msg.isZero());
                    result = secondCodec->decode(&msg, &decoded, &correctedPosSet);
                    if (result==DUE) {
                        correctedPosSet.clear();
                    }
                    break;
                }
                it++;
                pos++;
            }
        } else if (overlapFewDQFaults.size()>0) {
            int faultPos = rand()%overlapFewDQFaults.size();
            int pos;
            for (auto it = overlapFewDQFaults.cbegin(); it != overlapFewDQFaults.cend();) {
                if (pos==faultPos) {
                    assert((*it)->getNumDQ()<=2);
                    int pinID = (*it)->getPinID();
                    for (int j=0; j<errorBlk.getBeatHeight(); j++) {
                        errorBlk.setBit(j*errorBlk.getChannelWidth()+pinID, 0);
                    }
                    correctedPosSet.insert(pinID);
                    if ((*it)->getNumDQ()==2) {
                        int pinID2 = (*it)->getPinID();
                        for (int j=0; j<errorBlk.getBeatHeight(); j++) {
                            errorBlk.setBit(j*errorBlk.getChannelWidth()+pinID2, 0);
                        }
                        correctedPosSet.insert(pinID2);
                    }
                    // error correction
                    ECCWord msg = {72*8, 64*8};
                    ECCWord decoded = {72*8, 64*8};

                    msg.extract(&errorBlk, PIN, 0, errorBlk.getChannelWidth());
                    assert(!msg.isZero());
                    result = fourthCodec->decode(&msg, &decoded, &correctedPosSet);
                    if (result==DUE) {
                        correctedPosSet.clear();
                    }
                    break;
                }
                it++;
                pos++;
            }
        }
    }

    //if ((result==DUE)||(result==SDC)) {
    //    Fault *newFault = fd->operationalFaultList.back();
    //    printf("BB %d ", result);
    //    for (auto it2=fd->operationalFaultList.cbegin(); it2!=fd->operationalFaultList.cend(); it2++) {
    //        if ((*it2)->overlap(newFault)) {
    //            printf("%s(%d/%d) ", (*it2)->name.c_str(), (*it2)->isTransient, (*it2)->numDQ);
    //        }
    //    }
    //    printf("\n");
    //    temp.print();
    //}
    return result;
}

unsigned long long QPC72bREGB::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        double goodSymProb = pow(1-cellFaultRate, fd->getBeatHeight());
        double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) + fd->getChannelWidth()*pow(goodSymProb, fd->getChannelWidth()-1)*(1-goodSymProb) + fd->getChannelWidth()*(fd->getChannelWidth()-1)/2*pow(goodSymProb, fd->getChannelWidth()-2)*pow(1-goodSymProb, 2);

        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/64;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

