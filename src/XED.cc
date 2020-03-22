#include "XED.hh"
#include "FaultDomain.hh"

//------------------------------------------------------------------------------
CRC8_ATM::CRC8_ATM(const char *name, int _bitN, int _bitR)
: Codec(name, _bitN, _bitR)
{
    Block testBlock(72);
    ECCWord encoded(72, 64);

    for (int i=0; i<256; i++) {
        correctionTable[i] = 255;
    }

    for (int i=0; i<64; i++) {
        testBlock.reset();
        testBlock.setBit(i, 1);

        encode(&testBlock, &encoded);

        unsigned char crc = 0;
        for (int j=bitK; j<bitN; j++) {
            crc = (crc<<1) | encoded.getBit(j);
        }
        correctionTable[crc] = i;
    }
}

void CRC8_ATM::encode(Block *data, ECCWord *encoded) {
    bool CRC[8] = {false, };

    encoded->reset();

    for (int i=0; i<bitK; i++) {
        bool doInvert = data->getBit(i) ^ CRC[7];

        //// polynomial x^8 + x^5 + x^4 + 1
        //// 100110001
        //CRC[7] = CRC[6];
        //CRC[6] = CRC[5];
        //CRC[5] = CRC[4] ^ doInvert;
        //CRC[4] = CRC[3] ^ doInvert;
        //CRC[3] = CRC[2];
        //CRC[2] = CRC[1];
        //CRC[1] = CRC[0];
        //CRC[0] = doInvert;
        // polynomial x^8 + x^2 + x + 1
        // 100000111
        CRC[7] = CRC[6];
        CRC[6] = CRC[5];
        CRC[5] = CRC[4];
        CRC[4] = CRC[3];
        CRC[3] = CRC[2];
        CRC[2] = CRC[1] ^ doInvert;
        CRC[1] = CRC[0] ^ doInvert;
        CRC[0] = doInvert;
    }
    for (int i=0; i<bitK; i++) {
        encoded->setBit(i, data->getBit(i));
    }
    for (int i=bitK; i<bitN; i++) {
        encoded->setBit(i, CRC[i-bitK]);
    }
}

ErrorType CRC8_ATM::decode(ECCWord *msg, ECCWord *decoded, std::set<int>* correctedPos) {
    bool CRC[8] = {false, };

    decoded->reset();

    bool isDataAllZero = true;
    for (int i=0; i<bitK; i++) {
        bool data = msg->getBit(i);
        if (data) {
            isDataAllZero = false;
        }
        decoded->setBit(i, data);
        bool doInvert = data ^ CRC[7];

        //// polynomial x^8 + x^5 + x^4 + 1
        //// 100110001
        //CRC[7] = CRC[6];
        //CRC[6] = CRC[5];
        //CRC[5] = CRC[4] ^ doInvert;
        //CRC[4] = CRC[3] ^ doInvert;
        //CRC[3] = CRC[2];
        //CRC[2] = CRC[1];
        //CRC[1] = CRC[0];
        //CRC[0] = doInvert;
        // polynomial x^8 + x^2 + x + 1
        // 100000111
        CRC[7] = CRC[6];
        CRC[6] = CRC[5];
        CRC[5] = CRC[4];
        CRC[4] = CRC[3];
        CRC[3] = CRC[2];
        CRC[2] = CRC[1] ^ doInvert;
        CRC[1] = CRC[0] ^ doInvert;
        CRC[0] = doInvert;
    }
    unsigned char crc = 0;
    bool isCheckAllZero = true;
    for (int i=bitK; i<bitN; i++) {
        crc = (crc<<1) | CRC[i-bitK];

        bool check = msg->getBit(i);
        if (check) {
            isCheckAllZero = false;
        }
        decoded->setBit(i, 0);
    }
    if (crc==0) {
        if (isDataAllZero && !isCheckAllZero) {
            // errors on checksum bits only
            return CE;
        } else {
            return NE;
        }
    } else {
        if (correctionTable[crc]!=255) {
            decoded->invBit(correctionTable[crc]);
            return CE;
        } else {
            return DUE;
        }
    }
}

//------------------------------------------------------------------------------
XED::XED(bool _doFaultDiagnosis) : ECC(), doFaultDiagnosis(_doFaultDiagnosis) {
    onchip_codec = new CRC8_ATM("CRC8-ATM\t18\t4\t", 72, 8);
}

ErrorType XED::diagnoseFault(FaultDomain *fd, CacheLine &errorBlk, int erasures) {
    Fault *newFault = fd->operationalFaultList.back();

    std::list<Fault *>overlapFaults;
    // inter-/intra-block diagnosis
    for (auto it=fd->operationalFaultList.cbegin(); it!=fd->operationalFaultList.cend(); it++) {
        if ((*it)->overlap(newFault)) {
            if ((*it)->getIsSingleDQ() && (*it)->getIsSingleBeat()) {
                // single bit error
                // -> cannot diagnoze
            } else if ((*it)->getIsTransient() && !(*it)->getIsMultiColumn() && !(*it)->getIsMultiRow()) {
                // transient single-word fault
                // -> cannot diagnoze
            } else {
                overlapFaults.push_back(*it);
            }
        }
    }
    if (overlapFaults.size()!=0) {
        int faultPos = rand()%overlapFaults.size();
        int pos = 0;
        int correctCount = 0;
        for (auto it = overlapFaults.cbegin(); it != overlapFaults.cend(); ) {
            if (pos==faultPos) {
                // erasure correction
                int chipID = (*it)->getChipID();
                for (int i=0; i<errorBlk.getChipWidth(); i++) {
                    for (int j=0; j<errorBlk.getBeatHeight(); j++) {
                        errorBlk.setBit(j*errorBlk.getChannelWidth()+chipID*errorBlk.getChipWidth()+i, 0);
                    }
                    correctedPosSet.insert(chipID*4+i);
                }
                correctCount++;
                if (correctCount>=erasures) {
                    break;
                }
            }
            it++;
            pos++;
        }
    }
    if (errorBlk.isZero()) {
        return CE;
    } else {
        return DUE;
    }
}

void XED::detectInDRAM(CacheLine &errorBlk, std::list<int> &chipLocations) {
    ECCWord msg = {72, 64};
    ECCWord decoded = {72, 64};

    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);

            if ((result==CE)||(result==DUE)) {
                // catch-word collision
                unsigned long long draw = ((unsigned long long)rand()*((unsigned long long)RAND_MAX+1))| rand();
                if (draw%0x100000000ull!=0) {    // 2^-32
                    // no collision
                    chipLocations.push_back(i);
                }
            }
        }
    }
}

void XED::correctInDRAM(CacheLine &errorBlk) {
    ECCWord msg = {72, 64};
    ECCWord decoded = {72, 64};

    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
            if (result==CE) {
                for (int j=0; j<64; j++) {
                    if (msg.getBit(j)!=decoded.getBit(j)) {
                        errorBlk.invBit(i*4 + j%4 + (j/4)*errorBlk.getChannelWidth());
                    }
                }
                // ignore in-DRAM redundant bits
                for (int j=64; j<72; j++) {
                    errorBlk.setBit(i*4 + j%4 + (j/4)*errorBlk.getChannelWidth(), 0);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
ErrorType XED_SDDC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    // on-chip ECC
    if (errorBlk.isZero()) {
        return NE;
    }

    // in-DRAM ECC detection
    std::list<int> chipErrorDetectionLocations;
    detectInDRAM(errorBlk, chipErrorDetectionLocations);

    // 1. no detection
    if (chipErrorDetectionLocations.size()==0) {
        // Paper Section VI
        // 1.1. check parity
        bool match = checkParity(errorBlk);

        if (match) {
            return SDC;
        } else {
            // detected by parity
            if (doFaultDiagnosis) {
                // fault diagnosis + erasure decoding
                ErrorType result = diagnoseFault(fd, errorBlk, 1);
                return result;
            } else {
                return DUE;
            }
        }
    } else if (chipErrorDetectionLocations.size()==1) {
        // Paper Section V.C.2
        // 1.1. erasure decoding
        for (auto it = chipErrorDetectionLocations.cbegin(); it != chipErrorDetectionLocations.cend(); it++) {
            int faultyChipLocation = (*it);
            // correct using erasure code
            for (int j=0; j<72; j++) {
                errorBlk.setBit(faultyChipLocation*4 + j%4 + (j/4)*errorBlk.getChannelWidth(), 0);
            }
        }
        
        if (errorBlk.isZero()) {
            return CE;
        } else {
            return SDC;
        }
    } else {
        // Paper Section VII
        // serial mode (SEC correction of on-die ECC)
        correctInDRAM(errorBlk);

        if (errorBlk.isZero()) {
            return CE;
        }

        // 1.2. check parity
        bool match = checkParity(errorBlk);

        if (match) {
            // (possibly) 1-bit error is corrected by SEC
            return SDC;
        } else {
            // detected by parity
            if (doFaultDiagnosis) {
                ErrorType result = diagnoseFault(fd, errorBlk, 1);
                return result;
            } else {
                return DUE;
            }
        }
    }
}

unsigned long long XED_SDDC::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
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

bool XED_SDDC::checkParity(CacheLine &errorBlk) {
    ECCWord parity = {72, 64};
    ECCWord msg = {72, 64};

    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            parity^=msg;
        }
    }
    return parity.isZero();
}

//------------------------------------------------------------------------------
ErrorType XED_DDDC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    // on-chip ECC
    if (errorBlk.isZero()) {
        return NE;
    }

    // in-DRAM ECC detection
    std::list<int> chipErrorDetectionLocations;
    detectInDRAM(errorBlk, chipErrorDetectionLocations);

    // 1. no detection
    if (chipErrorDetectionLocations.size()==0) {
        // Paper Section VI
        // 1.1. check parity
        bool match = checkParity(errorBlk);

        if (match) {
            return SDC;
        } else {
            // detected by parity
            if (doFaultDiagnosis) {
                // fault diagnosis + erasure decoding
                ErrorType result = diagnoseFault(fd, errorBlk, 2);
                return result;
            } else {
                return DUE;
            }
        }
    } else if (chipErrorDetectionLocations.size()==1) {
        // Paper Section V.C.2
        // 1.1. erasure decoding
        for (auto it = chipErrorDetectionLocations.cbegin(); it != chipErrorDetectionLocations.cend(); it++) {
            int faultyChipLocation = (*it);
            // correct using erasure code
            for (int j=0; j<72; j++) {
                errorBlk.setBit(faultyChipLocation*4 + j%4 + (j/4)*errorBlk.getChannelWidth(), 0);
            }
        }

        if (errorBlk.isZero()) {
            return CE;
        }

        bool match = checkParity(errorBlk);
        if (match) {
            return SDC;
        } else {
            if (doFaultDiagnosis) {
                // fault diagnosis + erasure decoding
                ErrorType result = diagnoseFault(fd, errorBlk, 2);
                return result;
            } else {
                return DUE;
            }
        }
    } else if (chipErrorDetectionLocations.size()==2) {
        // Paper Section V.C.2
        // 1.1. erasure decoding
        for (auto it = chipErrorDetectionLocations.cbegin(); it != chipErrorDetectionLocations.cend(); it++) {
            int faultyChipLocation = (*it);
            // correct using erasure code
            for (int j=0; j<72; j++) {
                errorBlk.setBit(faultyChipLocation*4 + j%4 + (j/4)*errorBlk.getChannelWidth(), 0);
            }
        }
        
        if (errorBlk.isZero()) {
            return CE;
        } else {
            return SDC;
        }
    } else {
        // Paper Section VII
        // serial mode (SEC correction of on-die ECC)
        correctInDRAM(errorBlk);

        if (errorBlk.isZero()) {
            return CE;
        }

        // 1.2. check parity
        bool match = checkParity(errorBlk);

        if (match) {
            // (possibly) 1-bit error is corrected by SEC
            return SDC;
        } else {
            // detected by parity
            if (doFaultDiagnosis) {
                ErrorType result = diagnoseFault(fd, errorBlk, 2);
                return result;
            } else {
                return DUE;
            }
        }
    }
}

unsigned long long XED_DDDC::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
        assert(eccBlkSize==72);
        double goodECCBlkProb = pow(1-cellFaultRate, eccBlkSize) + eccBlkSize*pow(1-cellFaultRate, eccBlkSize-1) * cellFaultRate;
        double goodBlkProb = pow(goodECCBlkProb, fd->getChipCount()) + fd->getChipCount()*pow(goodECCBlkProb, fd->getChipCount()-1) * (1-goodECCBlkProb);
        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/128;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

bool XED_DDDC::checkParity(CacheLine &errorBlk) {
    ECCWord parity1 = {72, 64};
    ECCWord parity2 = {72, 64};
    ECCWord msg = {72, 64};

    for (int i=errorBlk.getChipCount()-1; i>=0; i--) {
        msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());

        if (!msg.isZero()) {
            if (i%2==0) {
                parity1^=msg;
            } else {
                parity2^=msg;
            }
        }
    }
    return parity1.isZero() & parity2.isZero();
}

