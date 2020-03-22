/**
 * @file: XED.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * DUO declaration 
 */

#ifndef __XED_HH__
#define __XED_HH__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <random>

#include "Config.hh"
#include "DomainGroup.hh"
#include "Tester.hh"
#include "Bamboo.hh"
#include "prior.hh"

//------------------------------------------------------------------------------
class CRC8_ATM: public Codec {
public:
    CRC8_ATM(const char *name, int _bitN, int _bitR);
    void encode(Block *data, ECCWord *encoded);
    ErrorType decode(ECCWord *msg, ECCWord *decoded, std::set<int>* correctedPos = NULL);
protected:
    unsigned char correctionTable[256];
};

//------------------------------------------------------------------------------
class XED: public ECC {
public:
    XED(bool _doFaultDiagnosis);
    ErrorType diagnoseFault(FaultDomain *fd, CacheLine &errorBlk, int erasures);
protected:
    void detectInDRAM(CacheLine &errorBlk, std::list<int> &chipLocations);
    void correctInDRAM(CacheLine &errorBlk);
    virtual bool checkParity(CacheLine &errorBlk) = 0;
    Codec *onchip_codec;
    bool doFaultDiagnosis;
};

class XED_SDDC: public XED {
public:
    XED_SDDC(bool _doFaultDiagnosis=true) : XED(_doFaultDiagnosis) {}
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
    bool needRetire(FaultDomain *fd, Fault *fault) { return !fault->getIsTransient() && (!fault->getIsSingleDQ() || !fault->getIsSingleBeat()); }
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    bool checkParity(CacheLine &errorBlk);
};

class XED_DDDC: public XED {
public:
    XED_DDDC(bool _doFaultDiagnosis=true) : XED(_doFaultDiagnosis) {}
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
    bool needRetire(FaultDomain *fd, Fault *fault) {
        if (!fault->getIsTransient()) {
            int chipCnt = 0;
            for (auto it = fd->operationalFaultList.cbegin(); it != fd->operationalFaultList.cend(); it++) {
                if ((*it)->overlap(fault)) {
                    if (!fault->getIsSingleDQ() || !fault->getIsSingleBeat()) {
                        chipCnt++;
                    }
                }
            }
            return (chipCnt>1);
        }
        return false;
    }
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    bool checkParity(CacheLine &errorBlk);
};

//------------------------------------------------------------------------------
#endif /* __XED_HH__ */
