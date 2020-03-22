/**
 * @file: Bamboo.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Bamboo-ECC declaration 
 */

#ifndef __BAMBOO_HH__
#define __BAMBOO_HH__

#include "rs.hh"
#include "ECC.hh"

//------------------------------------------------------------------------------
// SPC on 66b interface
//------------------------------------------------------------------------------
class SPC66bx4 : public ECC {
public:
    SPC66bx4();
};

//------------------------------------------------------------------------------
// SPC-TPD on 68b interface
//------------------------------------------------------------------------------
class SPCTPD68bx4 : public ECC {
public:
    SPCTPD68bx4();
};

//------------------------------------------------------------------------------
// QPC on 72b interface for x4 chipkill
//------------------------------------------------------------------------------
class QPC72b : public ECC {
public:
    QPC72b(int correction, int maxPins, bool _doPostprocess = true);
    QPC72b() : QPC72b(4, 2) {}

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);

protected:
    int maxPins;
};

//------------------------------------------------------------------------------
// QPC on 76b interface for AIECC
//------------------------------------------------------------------------------
class QPC76b : public ECC {
public:
    QPC76b(int correction, int maxPins, bool _doPostprocess = true);
    QPC76b() : QPC76b(4, 2) {}

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);

protected:
    int maxPins;
};

//------------------------------------------------------------------------------
// QPC on 80b interface for x8 chipkill
//------------------------------------------------------------------------------
class OPC80b: public ECC {
public:
    OPC80b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// OPC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
class OPC144b: public ECC {
public:
    OPC144b();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
// QPC with chip retirement
//------------------------------------------------------------------------------
class QPCCR: public ECC {
    QPCCR() {
        configList.push_back({0, 0, new RS<2, 8>("QPC\t18\t4\t", 72, 8, 4)});
        configList.push_back({0, 0, new RS<2, 8>("SPC-TPD\t17\t4\t", 68, 4, 1)});
    }
    ErrorType postprocess(FaultDomain *fd, ErrorType preResult) {
        if (fd->retiredChipIDList.size()<1) {
            if (correctedPosSet.size() > 2) {
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
        }
        return preResult;
    }
};

//------------------------------------------------------------------------------
// QPC with pin retirement
//------------------------------------------------------------------------------
class QPCPR: public ECC {
};
#endif /* __BAMBOO_HH__ */
