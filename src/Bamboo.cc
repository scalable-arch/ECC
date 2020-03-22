#include "Bamboo.hh"

//------------------------------------------------------------------------------
// SPC on 66b interface
//------------------------------------------------------------------------------
SPC66bx4::SPC66bx4() : ECC(PIN) {
    configList.push_back({0, 0, new RS<2, 8>("SPC\t17\t4\t", 66, 2, 1)});
}

//------------------------------------------------------------------------------
// SPC-TPD on 68b interface
//------------------------------------------------------------------------------
SPCTPD68bx4::SPCTPD68bx4() : ECC(PIN) {
    configList.push_back({0, 0, new RS<2, 8>("SPC-TPD\t17\t4\t", 68, 4, 1)});
}

//------------------------------------------------------------------------------
// QPC on 72b interface
//------------------------------------------------------------------------------
QPC72b::QPC72b(int correction, int _maxPins, bool _doPostprocess) : ECC(PIN, _doPostprocess) {
    maxPins = _maxPins;
    configList.push_back({0, 0, new RS<2, 8>("QPC\t18\t4\t", 72, 8, correction)});
}

ErrorType QPC72b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > maxPins) {
        int chipPos = -1;
        for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend(); it++) {
            int newChipPos = (*it)/4;
            if (chipPos == -1) {    // first chip location
                chipPos = newChipPos;
            } else {
                if (chipPos != newChipPos) {
                    correctedPosSet.clear();
                    return DUE;
                }
            }
        }
    }
    return preResult;
}

//------------------------------------------------------------------------------
// QPC on 76b interface (for AIECC)
//------------------------------------------------------------------------------
QPC76b::QPC76b(int correction, int _maxPins, bool _doPostprocess) : ECC(PIN, _doPostprocess) {
    maxPins = _maxPins;
    configList.push_back({0, 0, new RS<2, 8>("QPC\t19\t4\t", 76, 8, correction)});
}

ErrorType QPC76b::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > maxPins) {
        int chipPos = -1;
        for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend(); it++) {
            int newChipPos = (*it)/4;
            if (chipPos == -1) {    // first chip location
                chipPos = newChipPos;
            } else {
                if (chipPos != newChipPos) {
                    correctedPosSet.clear();
                    return DUE;
                }
            }
        }
    }
    return preResult;
}

//------------------------------------------------------------------------------
// QPC on 80b interface for x8 chipkill
//------------------------------------------------------------------------------
OPC80b::OPC80b() : ECC(PIN, true) {
    configList.push_back({0, 0, new RS<2, 8>("OPC80b\t10\t8\t", 80, 16, 8)});
}

ErrorType OPC80b::postprocess(FaultDomain *fd, ErrorType preResult) {
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
// OPC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
OPC144b::OPC144b() : ECC(PIN, true) {
    configList.push_back({0, 0, new RS<2, 8>("OPC144b\t18\t8\t", 144, 16, 8)});
}

ErrorType OPC144b::postprocess(FaultDomain *fd, ErrorType preResult) {
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
