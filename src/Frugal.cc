#include "Frugal.hh"

//------------------------------------------------------------------------------
// AMD Chipkill on 64-bit interface
//------------------------------------------------------------------------------
AMDChipkill64b::AMDChipkill64b() {
    layout = AMD;
    configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 16, 2, 1)});
}

ErrorType AMDChipkill64b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 1) {
        correctedPosSet.clear();
        return DUE;
    } else {
        return preResult;
    }
}

//------------------------------------------------------------------------------
// QPC on 64-bit interface
//------------------------------------------------------------------------------
QPC64b::QPC64b() {
    layout = PIN;
    configList.push_back({0, 0, new RS<2, 8>("QPC\t16\t4\t", 64, 8, 4)});
}

ErrorType QPC64b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 2) {
        int chipPos = -1;
        for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend(); it++) {
            int newChipPos = (*it)/4;
            if (chipPos==-1) {  // first
                chipPos = newChipPos;
            } else {
                if (newChipPos!=chipPos) {
                    correctedPosSet.clear();
                    return DUE;
                }
            }
        }
    }
    return preResult;
}

//------------------------------------------------------------------------------
// OPC on 72-bit interface
//------------------------------------------------------------------------------
OPC72b::OPC72b() {
    layout = PIN;
    configList.push_back({0, 0, new RS<2, 8>("OPC72b\t10\t8\t", 72, 16, 8)});
}
ErrorType OPC72b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 2) {
        int chipPos = -1;
        for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend(); it++) {
            int newChipPos = (*it)/8;
            if (chipPos==-1) {  // first
                chipPos = newChipPos;
            } else {
                if (newChipPos!=chipPos) {
                    correctedPosSet.clear();
                    return DUE;
                }
            }
        }
    }
    return preResult;
}

//------------------------------------------------------------------------------
// S8SC on 72-bit interface for x8 chipkill
//------------------------------------------------------------------------------
S8SC72b::S8SC72b() {
    layout = LINEAR;
    configList.push_back({0, 0, new RS<2, 8>("S8SC72b\t9\t8\t", 9, 2, 1)});
}

ErrorType S8SC72b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 1) {
        correctedPosSet.clear();
        return DUE;
    } else {
        return preResult;
    }
}
