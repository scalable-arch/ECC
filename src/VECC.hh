/**
 * @file: VECC.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Virtualized ECC declaration 
 */

#ifndef __VECC_HH__
#define __VECC_HH__

#include "ECC.hh"

#if 0
//------------------------------------------------------------------------------
class VECCST: public ECC {
public:
    VECCST(ECCLayout _layout) : ECC(_layout) {}

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
};

//------------------------------------------------------------------------------
// VECC on 128b interface for x4 chipkill
//------------------------------------------------------------------------------
class VECC128bx4: public VECCST {
public:
    VECC128bx4();
};

//------------------------------------------------------------------------------
// VECC on 128b interface for x8 chipkill
//------------------------------------------------------------------------------
class VECC128bx8: public VECCST {
public:
    VECC128bx8();
};

//------------------------------------------------------------------------------
class VECC64bAMD: public VECCST {
public:
    VECC64bAMD();
};

//------------------------------------------------------------------------------
class VECC64bQPC: public VECCST {
public:
    VECC64bQPC();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
class VECC64bS8SCD8SD: public VECCST {
public:
    VECC64bS8SCD8SD();
};

class VECC64bMultix4: public VECCST {
public:
    VECC64bMultix4();

    ErrorType decode(FaultDomain *fd, CacheLine &blk);
};


//------------------------------------------------------------------------------
class VECCTT: public ECC {
public:
    VECCTT() : ECC() {}

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);
protected:
    Codec *codec1;
    Codec *codec2;
};

//------------------------------------------------------------------------------
// 101: VECC on 136b interface for x4 chipkill
//------------------------------------------------------------------------------
class VECC136bx4: public VECCTT {
public:
    VECC136bx4();
};

//------------------------------------------------------------------------------
// 121: VECC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
class VECC144bx8: public VECCTT {
public:
    VECC144bx8();
};

//------------------------------------------------------------------------------
class VECC72bS8SCD8SD: public VECCTT {
public:
    VECC72bS8SCD8SD();
};
#endif

#endif /* __VECC_HH__ */
