#ifndef __CCA_ERROR_HH__
#define __CCA_ERROR_HH__

/**
 * @file: CCAError.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * CCAError declaration
 */

#include <string>

using namespace std;

enum CCAErrorType {
    NO_CCA_ERROR=0,
    CCA_ERROR_CMD=1,
    CCA_ERROR_TIMING=2,
    CCA_ERROR_ACT_EXTRA=3,
    CCA_ERROR_ACT_MISS=4,
    CCA_ERROR_WR_EXTRA=5,
    CCA_ERROR_WR_MISS=6,
    CCA_ERROR_WR_CHOP=7,
    CCA_ERROR_WR2RD=8,
    CCA_ERROR_WADDR=9,
    CCA_ERROR_RD_EXTRA=10,
    CCA_ERROR_RD_MISS=11,
    CCA_ERROR_RD_CHOP=12,
    CCA_ERROR_RD2WR=13,
    CCA_ERROR_RADDR=14,
    CCA_ERROR_ODT=15,
    END_CCA_ERROR=16
};

typedef struct {
    CCAErrorType type;
    string name;
    bool isError,
         isMDC,
         WCRCdetect,
         DECCdetect,
         FSMdetect,
         eWCRCdetect,
         eDECCdetect;
} CCAErrorInfo;

extern CCAErrorInfo ccaErrorInfo[];

enum CCAErrorResult {CCA_NE, CCA_SDC, CCA_SMDC, CCA_DE_PAR, CCA_DE_FSM, CCA_DE_WCRC, CCA_DE_DECC, CCA_DE_DECC_LATE, CCA_END_RESULT};

#endif /* __CCA_ERROR_HH__ */
