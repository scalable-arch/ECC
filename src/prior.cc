#include "prior.hh"
#include "rs.hh"
#include "hsiao.hh"
#include "FaultDomain.hh"

//------------------------------------------------------------------------------
// SEC-DED on 72-bit interface
//------------------------------------------------------------------------------
SECDED72b::SECDED72b() : ECC(LINEAR) {
    configList.push_back({0, 0, new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8)});
}

//------------------------------------------------------------------------------
// S4SCS-D4SD on 144b interface for x4 chipkill
//------------------------------------------------------------------------------
S4SCD4SD144b::S4SCD4SD144b() : ECC(LINEAR) {
    configList.push_back({0, 0, new RS<2, 4>("S4SCD4SD 144b\t", 36, 4, 1)});
}

//------------------------------------------------------------------------------
// S8SC on 80b interface for x8 chipkill
//------------------------------------------------------------------------------
S8SC80b::S8SC80b() : ECC(LINEAR, true) {
    configList.push_back({0, 0, new RS<2, 8>("S8SC80b\t10\t8\t", 10, 2, 1)});
}

ErrorType S8SC80b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 1) {
        correctedPosSet.clear();
        return DUE;
    } else {
        return preResult;
    }
}

//------------------------------------------------------------------------------
// S8SC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
S8SC144b::S8SC144b() : ECC(LINEAR, true) {
    configList.push_back({0, 0, new RS<2, 8>("S8SC144b\t18\t8\t", 18, 2, 1)});
}

ErrorType S8SC144b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 1) {
        correctedPosSet.clear();
        return DUE;
    } else {
        return preResult;
    }
}

//------------------------------------------------------------------------------
// AMD chipkill on 72b interface for x4 chipkill
//------------------------------------------------------------------------------
AMDChipkill72b::AMDChipkill72b(bool _doPostprocess) : ECC(AMD, _doPostprocess) {
    configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 18, 2, 1)});
}

ErrorType AMDChipkill72b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 1) {
        correctedPosSet.clear();
        return DUE;
    } else {
        return preResult;
    }
}

//------------------------------------------------------------------------------
// AMD double-chipkill on 144b interface for x4 chipkill
//------------------------------------------------------------------------------
AMDDChipkill144b::AMDDChipkill144b(bool _doPostprocess) : ECC(AMD, _doPostprocess) {
    configList.push_back({0, 0, new RS<2, 8>("D8SC w/ H (AMD)\t", 36, 4, 2)});
}

ErrorType AMDDChipkill144b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 2) {
        correctedPosSet.clear();
        return DUE;
    } else {
        return preResult;
    }
}

