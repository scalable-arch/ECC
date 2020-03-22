/**
 * @file: DUO.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * DUO declaration 
 */

#ifndef __DUO_HH__
#define __DUO_HH__

#include "rs.hh"
#include "ECC.hh"
#include "prior.hh"
#include "Bamboo.hh"

//------------------------------------------------------------------------------
class DUO64bx4 : public ECC {
public:
    DUO64bx4(int _maxPin);

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
    bool needRetire(FaultDomain *fd, Fault *fault) { return !fault->getIsTransient() && (correctedPosSet.size()>1)||(fault->getNumDQ()>1); }
protected:
    RS<2, 9> *codec;
    int maxPin;
};


class DUO36bx4 : public ECC {
public:
//    DUO36bx4(int _maxPin);
    DUO36bx4(int _maxPin, bool _doPostprocess, bool _doRetire, int _maxRetiredBlkCount);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault);
    bool needRetire(FaultDomain *fd, Fault *fault) { return !fault->getIsTransient() && (correctedPosSet.size()>1)||(fault->getNumDQ()>1); }
	bool ParityCheck(ECCWord *decoded, Block *errorBlk);
	void CorrectByParity(ECCWord *msg, Block *errorBlk, int faultyChip);
protected:
    RS<2, 8> *codec;
	RS_DUAL<2,8> *rs_dual_8;
	RS_DUAL<2,8> *rs_dual_10;
	std::list<int> *ErasureLocation;
    int maxPin;
};


#endif /* __DUO_HH__ */
