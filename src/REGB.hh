/**
 * @file: REGB.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * REGB declaration 
 */

#ifndef __REGB_HH__
#define __REGB_HH__

#include "rs.hh"
#include "ECC.hh"
#include "prior.hh"
#include "Bamboo.hh"

//------------------------------------------------------------------------------
class OnChip64b: public ECC {
public:
    OnChip64b();

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
protected:
    Codec *onchip_codec;
};

//------------------------------------------------------------------------------
class OnChip72bSECDED: public SECDED72b {
public:
    OnChip72bSECDED();

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
protected:
    Codec *onchip_codec;
};

//------------------------------------------------------------------------------
class OnChip72bAMD: public AMDChipkill72b {
public:
    OnChip72bAMD(bool _doPostprocess = true);

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
    bool needRetire(FaultDomain *fd, Fault *fault) { return !fault->getIsTransient() && (!fault->getIsSingleDQ() || !fault->getIsSingleBeat()); }
protected:
    Codec *onchip_codec;
};

//------------------------------------------------------------------------------
class OnChip72bQPC72b: public QPC72b {
public:
    OnChip72bQPC72b(int correction, int maxChips);

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
protected:
    Codec *onchip_codec;
};

//------------------------------------------------------------------------------
class QPC72bREGB: public QPC72b {
public:
    QPC72bREGB(bool _doFaultDiagnosis, bool _doRetire);

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
    bool needRetire(FaultDomain *fd, Fault *fault) { return !fault->getIsTransient() && (correctedPosSet.size()>2)||(fault->getNumDQ()>2); }

protected:
    bool doFaultDiagnosis;
    Codec *secondCodec;
    Codec *thirdCodec;
    Codec *fourthCodec;
};

#endif /* __REGB_HH__ */
