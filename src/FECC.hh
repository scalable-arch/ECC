/**
 * @file: FECC.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Frugal ECC declaration 
 */

#ifndef __FECC_HH__
#define __FECC_HH__

#include "ECC.hh"

//------------------------------------------------------------------------------
class FrugalECC : public ECC {
public:
    FrugalECC() : ECC() {}

    bool isHammingDistance(int origValue, int newValue, int distance);
};

//------------------------------------------------------------------------------
// Frugal ECC + QPC on 64b interface for x4 chipkill
//------------------------------------------------------------------------------
class FrugalECC64b : public FrugalECC {
public:
    FrugalECC64b();
    ~FrugalECC64b();

    virtual bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);

protected:
    Codec *codec64;
    Codec *codec642;
    Codec *codec68;
    Codec *codec73;
    
    Codec *codec_compressed;
    Codec *codec_un_compressed;
};

class FrugalECC64bNoEFP : public FrugalECC64b {
public:
    FrugalECC64bNoEFP() : FrugalECC64b() {}
    ~FrugalECC64bNoEFP() {}

    bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
};

//------------------------------------------------------------------------------
class FrugalECC64b2 : public FrugalECC {
public:
    FrugalECC64b2();
    ~FrugalECC64b2();

    virtual bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
protected:
    Codec *codec64;
    Codec *codec69;
};

//------------------------------------------------------------------------------
class FrugalECC64b3 : public FrugalECC {
public:
    FrugalECC64b3();
    ~FrugalECC64b3();

    virtual bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
protected:
    Codec *codec64;
    Codec *codec642;
    Codec *codec72;
    Codec *codec81;
};

//------------------------------------------------------------------------------
class FrugalECC72bOPC : public FrugalECC {
public:
    FrugalECC72bOPC();
    ~FrugalECC72bOPC();

    virtual bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);

protected:
    Codec *codec72;
    Codec *codec722;
    Codec *codec80;
    Codec *codec81;
};

class FrugalECC72bNoEFP : public FrugalECC72bOPC {
public:
    FrugalECC72bNoEFP() : FrugalECC72bOPC() {}
    ~FrugalECC72bNoEFP() {}

    bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
};

class FrugalECC64bMultix8 : public FrugalECC64b {
public:
    FrugalECC64bMultix8();

    bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
};

class FrugalECC64bMultix4 : public FrugalECC64bMultix8 {
public:
    FrugalECC64bMultix4();

//    bool decodeEF(CacheLine &errorBlk, int& origCase, int& newCase);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
};

#endif /* __FECC_HH__ */
