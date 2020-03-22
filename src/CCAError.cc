/**
 * @file: CCA.cc
 * @author: Jungrae Kim <dale40@gmail.com>
 * CCA implementation
 */

#include "CCAError.hh"

CCAErrorInfo ccaErrorInfo[] = {
    //                                     isErr, isMDC, WCRC,  DECC,  FSM,   eWCRC, eDECC
    {NO_CCA_ERROR,        "No error     ", false, false, false, false, false, false, false},
    {CCA_ERROR_CMD,       "CMD error    ", true,  true,  false, false, true,  false, false},
    {CCA_ERROR_TIMING,    "Timing error ", true,  true,  false, false, true,  false, false},
    {CCA_ERROR_ACT_EXTRA, "ACT+ error   ", true,  true,  false, false, true,  false, false},
    {CCA_ERROR_ACT_MISS,  "ACT- error   ", true,  true,  false, false, true,  false, false},
    {CCA_ERROR_WR_EXTRA,  "WR+ error    ", true,  true,  true,  false, false, true,  false},
    {CCA_ERROR_WR_MISS,   "WR- error    ", true,  true,  false, false, false, false, false},
    {CCA_ERROR_WR_CHOP,   "WRchop error ", true,  true,  true,  false, false, true,  true },
    {CCA_ERROR_WR2RD,     "WR2RD error  ", true,  true,  false, false, false, false, true },
    {CCA_ERROR_WADDR,     "WA error     ", true,  true,  false, false, false, true,  false},
    {CCA_ERROR_RD_EXTRA,  "RD+ error    ", true,  false, false, false, false, false, true },
    {CCA_ERROR_RD_MISS,   "RD- error    ", true,  false, false, false, false, false, true },
    {CCA_ERROR_RD_CHOP,   "RDchop error ", true,  false, false, false, false, false, true },
    {CCA_ERROR_RD2WR,     "RD2WR error  ", true,  true,  true,  false, false, true,  true },
    {CCA_ERROR_RADDR,     "RA error     ", true,  false, false, false, false, false, true },
    {CCA_ERROR_ODT,       "ODT error    ", true,  true,  true,  true,  false, true,  true },
    {END_CCA_ERROR,       "END          ", false, false, false, false, false, false, false}
};

