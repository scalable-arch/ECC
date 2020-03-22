/**
 * @file: Frugal.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Frugal-ECC declaration 
 */

#ifndef __FRUGAL_HH__
#define __FRUGAL_HH__

#include "rs.hh"
#include "ECC.hh"

//------------------------------------------------------------------------------
// AMD Chipkill on 64-bit interface
//------------------------------------------------------------------------------
class AMDChipkill64b : public ECC {
public:
    AMDChipkill64b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// QPC on 64-bit interface
//------------------------------------------------------------------------------
class QPC64b : public ECC {
public:
    QPC64b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// OPC on 72-bit interface
//------------------------------------------------------------------------------
class OPC72b: public ECC {
public:
    OPC72b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// S8SC on 72-bit interface for x8 chipkill
//------------------------------------------------------------------------------
class S8SC72b: public ECC {
public:
    S8SC72b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

#endif /* __FRUGAL_HH__ */
