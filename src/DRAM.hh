#ifndef __DRAM_HH__
#define __DRAM_HH__

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <iostream>
#include <bitset>
#include <string>
#include <list>

#include "CCAError.hh"
#include "common.hh"

#define ROW_ADDR 10
#define RD_ADDR  20
#define WR_ADDR  30

#define CA_SIZE 24
#define CCA_SIZE 27

//#define DBG_DALE(...) __VA_ARGS__
#define DBG_DALE(...) 

using namespace std;

extern long cur_cycle;

class DRAM {
public:
    DRAM(string name): m_name(name) {}

    string get_name() { return m_name; }

    virtual void init();
    virtual bool excludePAR() { return false; }
    //--------------------------------------------------------------------
    // send interface
    //--------------------------------------------------------------------
    CCAErrorResult activate(unsigned bg, unsigned ba, unsigned row,  unsigned error);
    CCAErrorResult wr_bl8(unsigned bg, unsigned ba, unsigned column, unsigned error);
    CCAErrorResult rd_bl8(unsigned bg, unsigned ba, unsigned column, unsigned error);
    CCAErrorResult precharge(unsigned bg, unsigned ba, unsigned error);
    CCAErrorResult precharge_all(unsigned error);
    CCAErrorResult ref(unsigned error);
    CCAErrorResult nop(unsigned error);
    //--------------------------------------------------------------------
    virtual CCAErrorResult send(std::bitset<CA_SIZE>& ca, unsigned error) = 0;
    //--------------------------------------------------------------------
    // receive interface
    //--------------------------------------------------------------------
    virtual CCAErrorResult receive(std::bitset<CCA_SIZE>& cca) = 0;
    virtual CCAErrorResult check() = 0;
    //--------------------------------------------------------------------
    CCAErrorType receive_internal(std::bitset<CCA_SIZE>& cca);
    CCAErrorType mrs_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType pre_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType prea_internal();
    CCAErrorType ref_internal();
    CCAErrorType act_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType wr_bl8_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType wra_bl8_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType rd_bl8_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType rda_bl8_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType zqcl_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType zqcs_internal(std::bitset<CA_SIZE>& ca);
    CCAErrorType rfu_internal();
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    void error_cmd(string str);
    void error_state(string str);
    void error_timing(string str);
    void error_address(string str);
    //--------------------------------------------------------------------
    CCAErrorType sb_check() {
        if (sb.size()>1) {
            auto it = sb.front();
            if (it.first) { // write
                return CCA_ERROR_WR_MISS;
            } else {
                return CCA_ERROR_RD_MISS;
            }
        }
        return NO_CCA_ERROR;
    }

protected:
    enum Position {CKE_POS=26, CS_POS=25, ODT_POS=24, PAR_POS=23, ACT_POS=22, RAS_POS=21, A16_POS=21, CAS_POS=20, A15_POS=20, WE_POS=19, A14_POS=19, BG1_POS=18, BG0_POS=17, BA1_POS=16, BA0_POS=15, A12_POS=14, BC_POS=14, A17_POS=13, A13_POS=12, A11_POS=11, A10_POS=10, AP_POS=10};
    enum State {CLOSED=0, OPEN=1};

    string m_name;

    unsigned m_n_bg;    // number of bank group
    unsigned m_n_bk;    // number of banks per bank group
    unsigned m_page_size;
    unsigned m_BL;
    // DDR timing parameters in time domain
    double m_tCK, m_tRCD, m_tRP, m_tRAS, m_tRC, m_tRRD_S, m_tRRD_L, m_tFAW;
    double m_tWR, m_tWR_CRC_DM;
    double m_tWTR_L, m_tWTR_L_CRC_DM;
    double m_tWTR_S, m_tWTR_S_CRC_DM;
    double m_tRTP, m_tCCD_S, m_tCCD_L, m_tRFC, m_tREFI;
    // DDR timing parameters in clock domain
    unsigned m_CL;
    unsigned m_AL;
    unsigned m_CWL;
    unsigned m_WL;
    unsigned m_RCD, m_RP, m_RAS, m_RC, m_RRD_S, m_RRD_L, m_FAW;
    unsigned m_WR, m_WR_CRC_DM;
    unsigned m_WTR_L, m_WTR_L_CRC_DM;
    unsigned m_WTR_S, m_WTR_S_CRC_DM;
    unsigned m_RTP, m_CCD_S, m_CCD_L, m_RFC, m_REFI;

    //unsigned m_WL;
    //unsigned m_REFI;

    // Timing checks
    State m_state[4][4];
    long m_last_act_cycle[4][4];
    long m_last_any_act_cycle[4];
    long m_last_wr_cycle[4][4];
    long m_last_any_wr_cycle;
    long m_last_rd_cycle[4][4];
    long m_last_any_rd_cycle;
    long m_last_pre_cycle[4][4];
    long m_last_ref_cycle[4][4];    // per-bank to support per-bank refresh of LPDDRx
    unsigned m_row_addr_tx[4][4];
    unsigned m_row_addr_rx[4][4];

    // Score board
    std::list<std::pair<bool, unsigned>> sb;
    unsigned m_addr_error;
};

class DDR3: public DRAM {
public:
    DDR3(): DRAM("DDR4-R") {}

    bool excludePAR() { return true; }

    CCAErrorResult send(std::bitset<CA_SIZE>& ca, unsigned error);
    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4: public DRAM {
public:
    DDR4(): DRAM("DDR4-Spec") {}

    CCAErrorResult send(std::bitset<CA_SIZE>& ca, unsigned error);
    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4a: public DRAM {
public:
    DDR4a(): DRAM("DDR4-Azul") {}

    CCAErrorResult send(std::bitset<CA_SIZE>& ca, unsigned error);
    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4n: public DRAM {
public:
    DDR4n(): DRAM("DDR4-Nick") {}

    CCAErrorResult send(std::bitset<CA_SIZE>& ca, unsigned error);
    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e: public DRAM {
public:
    DDR4e(): DRAM("DDR4-AI") {}
    DDR4e(string name): DRAM(name) {}
    void init() { DRAM::init(); tx_wr_toggle = false; rx_wr_toggle = false; }

    CCAErrorResult send(std::bitset<CA_SIZE>& ca, unsigned error);
    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
protected:
    bool tx_wr_toggle;
    bool rx_wr_toggle;
};

class DDR4e1: public DDR4e {    // ePAR only
public:
    DDR4e1(): DDR4e("DDR4-AI-1") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e2: public DDR4e {    // WCRC only
public:
    DDR4e2(): DDR4e("DDR4-AI-2") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e3: public DDR4e {    // DECC only
public:
    DDR4e3(): DDR4e("DDR4-AI-3") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e4: public DDR4e {    // FSM only
public:
    DDR4e4(): DDR4e("DDR4-AI-4") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e5: public DDR4e {    // WCRC + DECC
public:
    DDR4e5(): DDR4e("DDR4-AI-5") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e6: public DDR4e {    // PAR + FSM
public:
    DDR4e6(): DDR4e("DDR4-AI-6") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e7: public DDR4e {    // PAR + WCRC + DECC
public:
    DDR4e7(): DDR4e("DDR4-AI-7") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

class DDR4e8: public DDR4e {    // All
public:
    DDR4e8(): DDR4e("DDR4-AI-8") {}

    CCAErrorResult receive(std::bitset<CCA_SIZE>& cca);
    CCAErrorResult check();
};

// Micron DDR4 SDRAM (MT40A1G4-083)
// 4Gb (1Gb x x4) DDR4-2400
// - Number of bank groups: 4
// - Bank count per group: 4
// - Row addressing: 64K (A[15:0])
// - Column addressing: 1K (A[9:0])
// - Page size: 512B
class MT40A1G4_083: public DDR4 {
public:
    MT40A1G4_083(): DDR4() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083a: public DDR4a {
public:
    MT40A1G4_083a(): DDR4a() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e: public DDR4e {
public:
    MT40A1G4_083e(): DDR4e() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e1: public DDR4e1 {
public:
    MT40A1G4_083e1(): DDR4e1() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e2: public DDR4e2 {
public:
    MT40A1G4_083e2(): DDR4e2() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e3: public DDR4e3 {
public:
    MT40A1G4_083e3(): DDR4e3() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e4: public DDR4e4 {
public:
    MT40A1G4_083e4(): DDR4e4() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e5: public DDR4e5 {
public:
    MT40A1G4_083e5(): DDR4e5() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e6: public DDR4e6 {
public:
    MT40A1G4_083e6(): DDR4e6() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e7: public DDR4e7 {
public:
    MT40A1G4_083e7(): DDR4e7() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

class MT40A1G4_083e8: public DDR4e8 {
public:
    MT40A1G4_083e8(): DDR4e8() {
        m_n_bg          = 4;
        m_n_bk          = 4;
        m_page_size     = 512;      // 512B
        m_BL            = 8;
        m_CL            = 17;
        m_WL            = 12;
        m_tCK           = 0.833;
        m_tRCD          = 14.16;
        m_tRP           = 14.16;
        m_tRAS          = 32.0;
        m_tRC           = m_tRAS+m_tRP;
        m_tRRD_S        = 3.3;  // different bank group for 512B page size
        m_tRRD_L        = 4.9;  // same bank group for 512B page size
        m_tFAW          = 13.0; // 512B page size
        m_tWR           = 15.0; // 1tCK write preamble
        m_tWR_CRC_DM    = m_tWR + 5*m_tCK;
        m_tWTR_L        = 7.5;  // same bank group
        m_tWTR_L_CRC_DM = m_tWTR_L + 5*m_tCK;
        m_tWTR_S        = 2.5;  // different bank group
        m_tWTR_S_CRC_DM = m_tWTR_S + 5*m_tCK;
        m_tRTP          = 7.5;
        m_tCCD_S        = 4*m_tCK;
        m_tCCD_L        = 6*m_tCK;
        m_tRFC          = 260.0;     // all bank
        m_tREFI         = 7800.0;
    }
};

#endif /* __DRAM_HH__ */
