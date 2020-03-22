/**
 * @file: AIECC.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * All-Inclusive ECC declaration 
 */

#ifndef __AIECC_HH__
#define __AIECC_HH__

#include "ECC.hh"
#include "DRAM.hh"
#include "Bamboo.hh"

#ifdef AIECC
//------------------------------------------------------------------------------
class CCAECC : public ECC {
public:
    CCAECC(ECC *_dataECC, DRAM* _dram): dataECC(_dataECC), dram(_dram) {}

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);

protected:
    ECC *dataECC;
    DRAM* dram;
};

class DDR4QPC72b: public CCAECC {
public:
    DDR4QPC72b() : CCAECC(new QPC72b(), new MT40A1G4_083()) {}
    ~DDR4QPC72b() { delete dataECC; delete dram; }

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);
};

//------------------------------------------------------------------------------
class AzulQPC72b : public CCAECC {
public:
    AzulQPC72b() : CCAECC(new QPC72b(), new MT40A1G4_083a()) {}
    ~AzulQPC72b() { delete dataECC; delete dram; }

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);
};

//------------------------------------------------------------------------------
class NickQPC72b : public CCAECC {
public:
    NickQPC72b() : CCAECC(new QPC72b(), new MT40A1G4_083a()) {}
    ~NickQPC72b() { delete dataECC; delete dram; }

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);
};

//------------------------------------------------------------------------------
class AIECCQPC72b : public CCAECC {
public:
    AIECCQPC72b() : CCAECC(new QPC76b(), new MT40A1G4_083e()) {}
    ~AIECCQPC72b() { delete dataECC; delete dram; }

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);
};

#endif
//------------------------------------------------------------------------------
#endif /* __AIECC_HH__ */
