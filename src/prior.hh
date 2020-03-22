/**
 * @file: prior.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Prior ECC work declaration
 */

#ifndef __PRIOR_HH__
#define __PRIOR_HH__

#include "ECC.hh"

//------------------------------------------------------------------------------
// SEC-DED on 72-bit interface
//------------------------------------------------------------------------------
class SECDED72b : public ECC {
public:
    SECDED72b();
};

//------------------------------------------------------------------------------
// S4SCS-D4SD on 144b interface for x4 chipkill
//------------------------------------------------------------------------------
class S4SCD4SD144b: public ECC {
public:
    S4SCD4SD144b();
};

//------------------------------------------------------------------------------
// S8SC on 80b interface for x8 chipkill
//------------------------------------------------------------------------------
class S8SC80b: public ECC {
public:
    S8SC80b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// S8SC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
class S8SC144b: public ECC {
public:
    S8SC144b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// AMD chipkill on 72b interface for x4 chipkill
//------------------------------------------------------------------------------
class AMDChipkill72b : public ECC {
public:
    AMDChipkill72b(bool _doPostprocess = true);

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

class AMDDChipkill144b : public ECC {
public:
    AMDDChipkill144b(bool _doPostprocess = true);

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

#endif /* __PRIOR_HH__ */
