#include "DRAM.hh"

long cur_cycle;

//--------------------------------------------------------------------
void DRAM::init() {
    m_WL = m_AL + m_CWL;
    // convert time values into clock count (round up)
    m_RCD           = (m_tRCD         + m_tCK - 0.001) / m_tCK;
    m_RP            = (m_tRP          + m_tCK - 0.001) / m_tCK;
    m_RAS           = (m_tRAS         + m_tCK - 0.001) / m_tCK; 
    m_RC            = (m_tRC          + m_tCK - 0.001) / m_tCK;
    m_RRD_S         = (m_tRRD_S       + m_tCK - 0.001) / m_tCK;
    m_RRD_L         = (m_tRRD_L       + m_tCK - 0.001) / m_tCK;
    m_FAW           = (m_tFAW         + m_tCK - 0.001) / m_tCK;
    m_WR            = (m_tWR          + m_tCK - 0.001) / m_tCK;
    m_WR_CRC_DM     = (m_tWR_CRC_DM   + m_tCK - 0.001) / m_tCK;
    m_WTR_L         = (m_tWTR_L       + m_tCK - 0.001) / m_tCK;
    m_WTR_L_CRC_DM  = (m_tWTR_L_CRC_DM+ m_tCK - 0.001) / m_tCK;
    m_WTR_S         = (m_tWTR_S       + m_tCK - 0.001) / m_tCK;
    m_WTR_S_CRC_DM  = (m_tWTR_S_CRC_DM+ m_tCK - 0.001) / m_tCK;
    m_RTP           = (m_tRTP         + m_tCK - 0.001) / m_tCK;
    m_CCD_S         = (m_tCCD_S       + m_tCK - 0.001) / m_tCK;
    m_CCD_L         = (m_tCCD_L       + m_tCK - 0.001) / m_tCK;
    m_RFC           = (m_tRFC         + m_tCK - 0.001) / m_tCK;
    m_REFI          = (m_tREFI        + m_tCK - 0.001) / m_tCK;

    assert(m_CCD_S >= m_BL/2);  // tCCD should be bigger than burst length

    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            m_state[i][j] = CLOSED;
            m_last_act_cycle[i][j] = LONG_MIN;
            m_last_wr_cycle[i][j]  = LONG_MIN;
            m_last_rd_cycle[i][j]  = LONG_MIN;
            m_last_pre_cycle[i][j] = LONG_MIN;
            m_last_ref_cycle[i][j] = LONG_MIN;
        }
    }
    m_last_any_wr_cycle = LONG_MIN;
    m_last_any_rd_cycle = LONG_MIN;
    m_last_any_act_cycle[3] = m_last_any_act_cycle[2] = m_last_any_act_cycle[1] = m_last_any_act_cycle[0] = LONG_MIN;

    sb.clear();
}

//--------------------------------------------------------------------
// send interface
//--------------------------------------------------------------------
CCAErrorResult DRAM::activate(unsigned bg, unsigned ba, unsigned row, unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   0,     RA,    RA,    RA,    BG,        BA,        RA,  RA,  RA,  RA,  RA,  RA
    std::bitset<CA_SIZE> ca(row);
    ca[ACT_POS] = 0;
    ca[BG1_POS] = (bg>>1)&1;
    ca[BG0_POS] = (bg>>0)&1;
    ca[BA1_POS] = (ba>>1)&1;
    ca[BA0_POS] = (ba>>0)&1;
    ca[A17_POS] = (row>>17)&1;
    ca[A16_POS] = (row>>16)&1;
    ca[A15_POS] = (row>>15)&1;
    ca[A14_POS] = (row>>14)&1;
    ca[A13_POS] = (row>>13)&1;
    ca[A12_POS] = (row>>12)&1;
    ca[A11_POS] = (row>>11)&1;
    ca[A10_POS] = (row>>10)&1;

    m_row_addr_tx[bg][ba] = row;
    return send(ca, error);
}

CCAErrorResult DRAM::wr_bl8(unsigned bg, unsigned ba, unsigned column, unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   1,     1,     0,     0,     BG,        BA,        V,   V,   V,   V,   L,   CA
    std::bitset<CA_SIZE> ca(column);
    ca[ACT_POS] = 1;
    ca[RAS_POS] = 1;
    ca[CAS_POS] = 0;
    ca[WE_POS]  = 0;
    ca[BG1_POS] = (bg>>1)&1;
    ca[BG0_POS] = (bg>>0)&1;
    ca[BA1_POS] = (ba>>1)&1;
    ca[BA0_POS] = (ba>>0)&1;
    ca[A17_POS] = 1;
    ca[A13_POS] = 1;
    ca[A12_POS] = 1;
    ca[A11_POS] = 1;
    ca[AP_POS]  = 0;

    unsigned addr = (bg<<30)|(ba<<28)|(m_row_addr_tx[bg][ba]<<10)|column;
    sb.push_back(std::make_pair(true, addr));
    return send(ca, error);
}

CCAErrorResult DRAM::rd_bl8(unsigned bg, unsigned ba, unsigned column, unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   1,     1,     0,     1,     BG,        BA,        V,   V,   V,   V,   L,   CA
    std::bitset<CA_SIZE> ca(column);
    ca[ACT_POS] = 1;
    ca[RAS_POS] = 1;
    ca[CAS_POS] = 0;
    ca[WE_POS]  = 1;
    ca[BG1_POS] = (bg>>1)&1;
    ca[BG0_POS] = (bg>>0)&1;
    ca[BA1_POS] = (ba>>1)&1;
    ca[BA0_POS] = (ba>>0)&1;
    ca[A17_POS] = 1;
    ca[A13_POS] = 1;
    ca[A12_POS] = 1;
    ca[A11_POS] = 1;
    ca[AP_POS]  = 0;

    unsigned addr = (bg<<30)|(ba<<28)|(m_row_addr_tx[bg][ba]<<10)|column;
    sb.push_back(std::make_pair(false, addr));
    return send(ca, error);
}

CCAErrorResult DRAM::precharge(unsigned bg, unsigned ba, unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   1,     0,     1,     0,     BG,        BA,        V,   V,   V,   V,   L,   V
    std::bitset<CA_SIZE> ca((1<<CA_SIZE)-1);   // All-high
    ca[ACT_POS] = 1;
    ca[RAS_POS] = 0;
    ca[CAS_POS] = 1;
    ca[WE_POS]  = 0;
    ca[BG1_POS] = (bg>>1)&1;
    ca[BG0_POS] = (bg>>0)&1;
    ca[BA1_POS] = (ba>>1)&1;
    ca[BA0_POS] = (ba>>0)&1;
    ca[A10_POS] = 0;
    return send(ca, error);
}

CCAErrorResult DRAM::precharge_all(unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   1,     0,     1,     0,     V,         V,         V,   V,   V,   V,   H,   V
    std::bitset<CA_SIZE> ca((1<<CA_SIZE)-1);   // All-high
    ca[ACT_POS] = 1;
    ca[RAS_POS] = 0;
    ca[CAS_POS] = 1;
    ca[WE_POS]  = 0;
    ca[A10_POS] = 1;
    return send(ca, error);
}

CCAErrorResult DRAM::ref(unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   1,     0,     0,     1,     V,         V,         V,   V,   V,   V,   V,   V
    std::bitset<CA_SIZE> ca((1<<CA_SIZE)-1);   // All-high
    ca[ACT_POS] = 1;
    ca[RAS_POS] = 0;
    ca[CAS_POS] = 0;
    ca[WE_POS]  = 1;
    return send(ca, error);
}

CCAErrorResult DRAM::nop(unsigned error) {
    //   ACT_n, RAS_n, CAS_n, WE_n,  BG[1:0],   BA[1:0],   A12, A17, A13, A11, A10, A[9:0]
    //          A16,   A15,   A14,                         BC_n,               AP
    //   1,     1,     1,     1,     V,         V,         V,   V,   V,   V,   V,   V
    std::bitset<CA_SIZE> ca((1<<CA_SIZE)-1);   // All-high
    ca[ACT_POS] = 1;
    ca[RAS_POS] = 1;
    ca[CAS_POS] = 1;
    ca[WE_POS]  = 1;
    return send(ca, error);
}

//--------------------------------------------------------------------
// receive interface
//--------------------------------------------------------------------
CCAErrorType DRAM::receive_internal(std::bitset<CCA_SIZE>& cca) {
    std::bitset<CA_SIZE> ca;
    DBG_DALE(printf("receive");)

    for (int i=0; i<PAR_POS; i++) {
        ca[i] = cca[i];
    }

    // check remainig RD/WR
    CCAErrorType result = sb_check();
    if (result!=NO_CCA_ERROR) {
        return result;
    }

    if (cca[CS_POS]||!cca[CKE_POS]) {
        // deselect or power save
        result = NO_CCA_ERROR;
    } else {
        // chip select is active (low)
        if (cca[ACT_POS] & !cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
            // MRS
            result = mrs_internal(ca);
        } else if (cca[ACT_POS] & !cca[RAS_POS] & !cca[CAS_POS] & cca[WE_POS]) {
            // refresh
            result = ref_internal();
        } else if (cca[ACT_POS] & !cca[RAS_POS] & cca[CAS_POS] & !cca[WE_POS]) {
            // precharge
            if (!cca[AP_POS]) {   // precharge
                result = pre_internal(ca);
            } else {                // precharge-all
                result = prea_internal();                        
            }
        } else if (cca[ACT_POS] & !cca[RAS_POS] & cca[CAS_POS] & cca[WE_POS]) {
            // RFU
            result = rfu_internal();
        } else if (!cca[ACT_POS]) {
            // activate
            result = act_internal(ca);
        } else if (cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
            if (!cca[BC_POS]) {
                return CCA_ERROR_WR_CHOP;
            } else {
                // write BL8
                if (!cca[AP_POS]) {   // without auto-precharge
                    result = wr_bl8_internal(ca);
                } else {                // with auto-precharge
                    result = wra_bl8_internal(ca);
                }
            }
        } else if (cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & cca[WE_POS]) {
            if (!cca[BC_POS]) {
                return CCA_ERROR_RD_CHOP;
            } else {
                // read BL8
                if (!cca[AP_POS]) {   // without auto-precharge
                    result = rd_bl8_internal(ca);
                } else {                // with auto-precharge
                    result = rda_bl8_internal(ca);
                }
            }
        } else if (cca[ACT_POS] & cca[RAS_POS] & cca[CAS_POS] & cca[WE_POS]) {
            // nop
            result = NO_CCA_ERROR;
        } else if (cca[ACT_POS] & cca[RAS_POS] & cca[CAS_POS] & !cca[WE_POS]) {
            // ZQ calibration
            if (cca[AP_POS]) {   // without auto-precharge
                result = zqcl_internal(ca);
            } else {
                result = zqcs_internal(ca);
            }
        } else {
            assert(0);
        }
    }

    if (cca[ODT_POS]) {
        if (result==NO_CCA_ERROR) {
            return CCA_ERROR_ODT;
        }
    }
    return result;
}


//--------------------------------------------------------------------
CCAErrorType DRAM::mrs_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("MRS\n");)
    return CCA_ERROR_CMD;
}

CCAErrorType DRAM::zqcl_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("ZQC\n");)
    return CCA_ERROR_CMD;
}

CCAErrorType DRAM::zqcs_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("ZQC\n");)
    return CCA_ERROR_CMD;
}

CCAErrorType DRAM::rfu_internal() {
    DBG_DALE(printf("RFU\n");)
    return CCA_ERROR_CMD;
}

CCAErrorType DRAM::ref_internal() {
    DBG_DALE(printf("REF\n");)
    // update
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            // state check
            if (m_state[i][j]==OPEN) {
                error_state("REF during OPEN");
                return CCA_ERROR_ACT_EXTRA;
            }

            // timing check
            // from previous PRE: tRP
            if ((m_last_pre_cycle[i][j]+m_RP) > cur_cycle) {
                error_timing("PRE->REF: tRP");
                return CCA_ERROR_TIMING;
            }
            // from previous REF: tRFC
            if ((m_last_ref_cycle[i][j]+m_RFC) > cur_cycle) {
                error_timing("REF->REF: tRFC");
                return CCA_ERROR_TIMING;
            }

            // update
            m_last_ref_cycle[i][j] = cur_cycle;
        }
    }

    return NO_CCA_ERROR;
}

CCAErrorType DRAM::act_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("ACT\n");)
    unsigned bg  = (ca[BG1_POS]<<1) | ca[BG0_POS];
    unsigned ba  = (ca[BA1_POS]<<1) | ca[BA0_POS];
    unsigned row = (ca[A17_POS]<<17) |
                   (ca[A16_POS]<<16) |
                   (ca[A15_POS]<<15) |
                   (ca[A14_POS]<<14) |
                   (ca[A13_POS]<<13) |
                   (ca[A12_POS]<<12) |
                   (ca[A11_POS]<<11) |
                   (ca[A10_POS]<<10) |
                   (ca[9]      <<9) |
                   (ca[8]      <<8) |
                   (ca[7]      <<7) |
                   (ca[6]      <<6) |
                   (ca[5]      <<5) |
                   (ca[4]      <<4) |
                   (ca[3]      <<3) |
                   (ca[2]      <<2) |
                   (ca[1]      <<1) |
                   (ca[0]      <<0);

    // state check
    if (m_state[bg][ba]==OPEN) {
        error_state("ACT during OPEN");
        return CCA_ERROR_ACT_EXTRA;
    }

    // timing check
    // from previous ACT (same bank)                : tRC
    // from previous ACT (diff bank same bank group): tRRD_L
    // from previous ACT (diff bank diff bank group): tRRD_S
    for (int i=0; i<m_n_bg; i++) {
        for (int j=0; j<m_n_bk; j++) {
            if (i==bg) {
                if (j==ba) {
                    if ((m_last_act_cycle[i][j]+m_RC) > cur_cycle) {
                        error_timing("ACT->ACT: tRC");
                        return CCA_ERROR_TIMING;
                    }
                } else {
                    if ((m_last_act_cycle[i][j]+m_RRD_L) > cur_cycle) {
                        error_timing("ACT->ACT: tRRD_L");
                        return CCA_ERROR_TIMING;
                    }
                }
            } else {
                if ((m_last_act_cycle[i][j]+m_RRD_S) > cur_cycle) {
                    error_timing("ACT->ACT: tRRD_S");
                    return CCA_ERROR_TIMING;
                }
            }
        }
    }
    // from previous ACT (4 activation window)      : tFAW
    if ((m_last_any_act_cycle[3]+m_FAW) > cur_cycle) {
        error_timing("ACT->ACT: tFAW");
        return CCA_ERROR_TIMING;
    }
    // from previous PRE: tRP
    if ((m_last_pre_cycle[bg][ba]+m_RP) > cur_cycle) {
        error_timing("PRE->ACT: tRP");
        return CCA_ERROR_TIMING;
    }
    // from previous REF: tRFC
    if ((m_last_ref_cycle[bg][ba]+m_RFC) > cur_cycle) {
        error_timing("REF->ACT: tRFC");
        return CCA_ERROR_TIMING;
    }

    // update
    m_state[bg][ba] = OPEN;
    m_row_addr_rx[bg][ba] = row;

    m_last_act_cycle[bg][ba] = cur_cycle;

    m_last_any_act_cycle[3] = m_last_any_act_cycle[2];
    m_last_any_act_cycle[2] = m_last_any_act_cycle[1];
    m_last_any_act_cycle[1] = m_last_any_act_cycle[0];
    m_last_any_act_cycle[0] = cur_cycle;

    return NO_CCA_ERROR;
}

CCAErrorType DRAM::wr_bl8_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("WR_BL8\n");)
    unsigned bg  = (ca[BG1_POS]<<1) | ca[BG0_POS];
    unsigned ba  = (ca[BA1_POS]<<1) | ca[BA0_POS];
    unsigned col = (ca[9]      <<9) |
                   (ca[8]      <<8) |
                   (ca[7]      <<7) |
                   (ca[6]      <<6) |
                   (ca[5]      <<5) |
                   (ca[4]      <<4) |
                   (ca[3]      <<3) |
                   (ca[2]      <<2) |
                   (ca[1]      <<1) |
                   (ca[0]      <<0);

    // state check
    if (m_state[bg][ba]==CLOSED) {
        error_state("WR  during CLOSED");
        return CCA_ERROR_ACT_MISS;
    }

    // timing check
    // from previous ACT                            : tRCD
    if ((m_last_act_cycle[bg][ba]+m_RCD) > cur_cycle) {
        error_timing("ACT->WR : tRCD");
        return CCA_ERROR_TIMING;
    }
    // from previous WR                             : tCCD (also covers burst length)
    for (int i=0; i<m_n_bg; i++) {
        for (int j=0; j<m_n_bk; j++) {
            if (i==bg) {
                if ((m_last_wr_cycle[i][j]+m_CCD_L) > cur_cycle) {
                    error_timing("WR ->WR : tCCD_L");
                    return CCA_ERROR_TIMING;
                }
            } else {
                if ((m_last_wr_cycle[i][j]+m_CCD_S) > cur_cycle) {
                    error_timing("WR ->WR : tCCD_S");
                    return CCA_ERROR_TIMING;
                }
            }
        }
    }
    // from previous RD                             : tCCD
    for (int i=0; i<m_n_bg; i++) {
        for (int j=0; j<m_n_bk; j++) {
            if (i==bg) {
                if ((m_last_rd_cycle[i][j]+m_CCD_L) > cur_cycle) {
                    error_timing("RD ->WR : tCCD_L");
                    return CCA_ERROR_TIMING;
                }
            } else {
                if ((m_last_rd_cycle[i][j]+m_CCD_S) > cur_cycle) {
                    error_timing("RD ->WR : tCCD_S");
                    return CCA_ERROR_TIMING;
                }
            }
        }
    }

    // address check
    unsigned addr = (bg<<30)|(ba<<28)|(m_row_addr_rx[bg][ba]<<10)|col;
    if (sb.empty()) {
        return CCA_ERROR_WR_EXTRA;
    } else {
        auto it = sb.front();
        sb.pop_front();
        if (it.first!=true) {   // read changed to write
            return CCA_ERROR_RD2WR;
        }
        m_addr_error = it.second^addr;
        if (m_addr_error) {
            error_address("WR  address");
            return CCA_ERROR_WADDR;
        }
    }

    // update
    m_last_any_wr_cycle = cur_cycle;
    m_last_wr_cycle[bg][ba] = cur_cycle;

    return NO_CCA_ERROR;
}

CCAErrorType DRAM::wra_bl8_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("WRA_BL8\n");)
    CCAErrorType result = wr_bl8_internal(ca);
    if (result!=NO_CCA_ERROR) {
        return result;
    }
    
    cur_cycle += (m_WL+m_BL/2+m_WR);
    result = pre_internal(ca);
    cur_cycle -= (m_WL+m_BL/2+m_WR);
    return result;
}

CCAErrorType DRAM::rd_bl8_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("RD_BL8\n");)
    unsigned bg  = (ca[BG1_POS]<<1) | ca[BG0_POS];
    unsigned ba  = (ca[BA1_POS]<<1) | ca[BA0_POS];
    unsigned col = (ca[9]      <<9) |
                   (ca[8]      <<8) |
                   (ca[7]      <<7) |
                   (ca[6]      <<6) |
                   (ca[5]      <<5) |
                   (ca[4]      <<4) |
                   (ca[3]      <<3) |
                   (ca[2]      <<2) |
                   (ca[1]      <<1) |
                   (ca[0]      <<0);

    // state check
    if (m_state[bg][ba]==CLOSED) {
        error_state("RD  during CLOSED");
        return CCA_ERROR_ACT_MISS;
    }

    // timing check
    // from previous ACT                            : tRCD
    if ((m_last_act_cycle[bg][ba]+m_RCD) > cur_cycle) {
        error_timing("ACT->RD : tRCD");
        return CCA_ERROR_TIMING;
    }
    // from previous WR
    for (int i=0; i<m_n_bg; i++) {
        for (int j=0; j<m_n_bk; j++) {
            if (i==bg) {
                // tCCD (also covers burst length
                if ((m_last_wr_cycle[i][j]+m_CCD_L) > cur_cycle) {
                    error_timing("WR ->RD : tCCD_L");
                    return CCA_ERROR_TIMING;
                }
                // tWTR
                if ((m_last_wr_cycle[i][j]+m_WL+m_BL/2+m_WTR_L) > cur_cycle) {
                    error_timing("WR ->RD : tWTR_L");
                    return CCA_ERROR_TIMING;
                }
            } else {
                if ((m_last_wr_cycle[i][j]+m_CCD_S) > cur_cycle) {
                    error_timing("WR ->RD : tCCD_S");
                    return CCA_ERROR_TIMING;
                }
                if ((m_last_wr_cycle[i][j]+m_WL+m_BL/2+m_WTR_S) > cur_cycle) {
                    error_timing("WR ->RD : tWTR_S");
                    return CCA_ERROR_TIMING;
                }
            }
        }
    }
    // from previous RD                             : tCCD
    for (int i=0; i<m_n_bg; i++) {
        for (int j=0; j<m_n_bk; j++) {
            if (i==bg) {
                if ((m_last_rd_cycle[i][j]+m_CCD_L) > cur_cycle) {
                    error_timing("RD ->RD : tCCD_L");
                    return CCA_ERROR_TIMING;
                }
            } else {
                if ((m_last_rd_cycle[i][j]+m_CCD_S) > cur_cycle) {
                    error_timing("RD ->RD : tCCD_S");
                    return CCA_ERROR_TIMING;
                }
            }
        }
    }

    // address check
    unsigned addr = (bg<<30)|(ba<<28)|(m_row_addr_rx[bg][ba]<<10)|col;
    if (sb.empty()) {
        return CCA_ERROR_RD_EXTRA;
    } else {
        auto it = sb.front();
        sb.pop_front();
        if (it.first==true) {   // write changed to read
            return CCA_ERROR_WR2RD;
        }
        m_addr_error = it.second^addr;
        if (m_addr_error) {
            error_address("RD  address");
            return CCA_ERROR_RADDR;
        }
    }

    // update
    m_last_any_rd_cycle = cur_cycle;
    m_last_rd_cycle[bg][ba] = cur_cycle;

    return NO_CCA_ERROR;
}

CCAErrorType DRAM::rda_bl8_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("RDA_BL8\n");)
    CCAErrorType result = rd_bl8_internal(ca);
    if (result!=NO_CCA_ERROR) {
        return result;
    }
    
    cur_cycle += m_RTP;
    result = pre_internal(ca);
    cur_cycle -= m_RTP;
    return result;
}

CCAErrorType DRAM::pre_internal(std::bitset<CA_SIZE>& ca) {
    DBG_DALE(printf("PRE\n");)
    unsigned bg  = (ca[BG1_POS]<<1) | ca[BG0_POS];
    unsigned ba  = (ca[BA1_POS]<<1) | ca[BA0_POS];

    // state check
    if (m_state[bg][ba]==CLOSED) {
        error_state("PRE during CLOSED");
        return CCA_ERROR_ACT_MISS;
    }
    
    // timing check
    // from previous ACT: tRAS
    if ((m_last_act_cycle[bg][ba]+m_RAS) > cur_cycle) {
        error_timing("ACT->PRE: tRAS");
        return CCA_ERROR_TIMING;
    }
    // from previous RD : tRTP
    if ((m_last_rd_cycle[bg][ba]+m_RTP) > cur_cycle) {
        error_timing("RD ->PRE: tRTP");
        return CCA_ERROR_TIMING;
    }
    // from previous WR : tWR
    if ((m_last_wr_cycle[bg][ba]+m_WL+m_BL/2+m_WR) > cur_cycle) {
        error_timing("WR ->PRE: tWR");
        return CCA_ERROR_TIMING;
    }

    // update
    m_state[bg][ba] = CLOSED;
    m_last_pre_cycle[bg][ba] = cur_cycle;

    return NO_CCA_ERROR;
}

CCAErrorType DRAM::prea_internal() {
    DBG_DALE(printf("PREA\n");)
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            // state check
            if (m_state[i][j]==CLOSED) {
                error_state("PRE during CLOSED");
                return CCA_ERROR_ACT_MISS;
            }
            
            // timing check
            // from previous ACT: tRAS
            if ((m_last_act_cycle[i][j]+m_RAS) > cur_cycle) {
                error_timing("ACT->PRE: tRAS");
                return CCA_ERROR_TIMING;
            }
            // from previous RD : tRTP
            if ((m_last_rd_cycle[i][j]+m_RTP) > cur_cycle) {
                error_timing("RD ->PRE: tRTP");
                return CCA_ERROR_TIMING;
            }
            // from previous WR : tWR
            if ((m_last_wr_cycle[i][j]+m_WL+m_BL/2+m_WR) > cur_cycle) {
                error_timing("WR ->PRE: tWR");
                return CCA_ERROR_TIMING;
            }
            // update
            m_state[i][j] = CLOSED;
            m_last_pre_cycle[i][j] = cur_cycle;
        }
    }

    return NO_CCA_ERROR;
}

//--------------------------------------------------------------------
void DRAM::error_cmd(string str) {
    //fprintf(stdout, "Invalid command %s\n", str.c_str());
}

void DRAM::error_state(string str) {
    //fprintf(stdout, "Invalid state %s\n", str.c_str());
}

void DRAM::error_timing(string str) {
    //fprintf(stdout, "Invalid timing %s\n", str.c_str());
}

void DRAM::error_address(string str) {
    //fprintf(stdout, "Invalid address %s\n", str.c_str());
}

//--------------------------------------------------------------------
// DDR3
//--------------------------------------------------------------------
CCAErrorResult DDR3::send(std::bitset<CA_SIZE>& ca, unsigned error) {
    std::bitset<CCA_SIZE> cca;
    for (int i=0; i<PAR_POS; i++) {
        cca[i] = ca[i];
    }
    cca[PAR_POS] = 0;   // unused
    cca[ODT_POS] = 0;   // low
    cca[CS_POS]  = 0;   // select
    cca[CKE_POS] = 1;   // clock enable

    // inject errors
    for (int i=0; i<CCA_SIZE; i++) {
        cca[i] = cca[i] ^ ((error>>i)&1);
    }

    // receive
    return receive(cca);
}

//--------------------------------------------------------------------
// receive interface
//--------------------------------------------------------------------
CCAErrorResult DDR3::receive(std::bitset<CCA_SIZE>& cca) {
    CCAErrorType type = receive_internal(cca);
    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR3::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
// DDR4
//--------------------------------------------------------------------
CCAErrorResult DDR4::send(std::bitset<CA_SIZE>& ca, unsigned error) {
    std::bitset<CCA_SIZE> cca;
    // generate CA parity
    bool par = false;
    for (int i=0; i<PAR_POS; i++) {
        par ^= ca[i];
        cca[i] = ca[i];
    }
    cca[PAR_POS] = par;
    cca[ODT_POS] = 0;   // low
    cca[CS_POS]  = 0;   // select
    cca[CKE_POS] = 1;   // clock enable

    // inject errors
    for (int i=0; i<cca.size(); i++) {
        cca[i] = cca[i] ^ ((error>>i)&1);
    }

    // receive
    return receive(cca);
}

//--------------------------------------------------------------------
// receive interface
//--------------------------------------------------------------------
CCAErrorResult DDR4::receive(std::bitset<CCA_SIZE>& cca) {
    // check CA parity
    bool par = false;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].WCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].DECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].WCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].DECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
// DDR4 with Azul protection
//--------------------------------------------------------------------
CCAErrorResult DDR4a::send(std::bitset<CA_SIZE>& ca, unsigned error) {
    std::bitset<CCA_SIZE> cca;

    // generate CA parity
    bool par = false;
        for (int i=0; i<PAR_POS; i++) {
        par ^= ca[i];
        cca[i] = ca[i];
    }
    cca[PAR_POS] = par;
    cca[ODT_POS] = 0;   // low
    cca[CS_POS]  = 0;   // select
    cca[CKE_POS] = 1;   // clock enable

    // inject errors
    for (int i=0; i<cca.size(); i++) {
        cca[i] = cca[i] ^ ((error>>i)&1);
    }

    // receive
    return receive(cca);
}

//--------------------------------------------------------------------
// receive interface
//--------------------------------------------------------------------
CCAErrorResult DDR4a::receive(std::bitset<CCA_SIZE>& cca) {
    // check CA parity
    bool par = false;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].WCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].DECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].eDECCdetect) {
            int error_lsb = -1;
            int error_msb = -1;
            for (int i=0; i<64; i++) {
                if ((m_addr_error>>i) & 1) {
                    error_lsb = i;
                    break;
                }
            }
            for (int i=63; i>=0; i--) {
                if ((m_addr_error>>i) & 1) {
                    error_msb = i;
                    break;
                }
            }
            int burst_length = error_msb - error_lsb;
            if ((burst_length<4)||(rand()%256!=0)) {
                // 100% detection
                if (ccaErrorInfo[type].isMDC && (type!=CCA_ERROR_WR2RD)) {
                    return CCA_DE_DECC_LATE;
                } else {
                    return CCA_DE_DECC;
                }
            } else {
                if (ccaErrorInfo[type].isMDC) {
                    return CCA_SMDC;
                } else {
                    return CCA_SDC;
                }
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4a::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].WCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].DECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].eDECCdetect) {
            int error_lsb = -1;
            int error_msb = -1;
            for (int i=0; i<64; i++) {
                if ((m_addr_error>>i) & 1) {
                    error_lsb = i;
                    break;
                }
            }
            for (int i=63; i>=0; i--) {
                if ((m_addr_error>>i) & 1) {
                    error_msb = i;
                    break;
                }
            }
            int burst_length = error_msb - error_lsb;
            if ((burst_length<4)||(rand()%256!=0)) {
                // 100% detection
                if (ccaErrorInfo[type].isMDC && (type!=CCA_ERROR_WR2RD)) {
                    return CCA_DE_DECC_LATE;
                } else {
                    return CCA_DE_DECC;
                }
            } else {
                if (ccaErrorInfo[type].isMDC) {
                    return CCA_SMDC;
                } else {
                    return CCA_SDC;
                }
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
// DDR4 with Nicholas protection
//--------------------------------------------------------------------
CCAErrorResult DDR4n::send(std::bitset<CA_SIZE>& ca, unsigned error) {
    std::bitset<CCA_SIZE> cca;

    // generate CA parity
    bool par = false;
        for (int i=0; i<PAR_POS; i++) {
        par ^= ca[i];
        cca[i] = ca[i];
    }
    cca[PAR_POS] = par;
    cca[ODT_POS] = 0;   // low
    cca[CS_POS]  = 0;   // select
    cca[CKE_POS] = 1;   // clock enable

    // inject errors
    for (int i=0; i<cca.size(); i++) {
        cca[i] = cca[i] ^ ((error>>i)&1);
    }

    // receive
    return receive(cca);
}

//--------------------------------------------------------------------
// receive interface
//--------------------------------------------------------------------
CCAErrorResult DDR4n::receive(std::bitset<CCA_SIZE>& cca) {
    // check CA parity
    bool par = false;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].WCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].DECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC && (type!=CCA_ERROR_WR2RD)) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4n::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].WCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].DECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC && (type!=CCA_ERROR_WR2RD)) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
// DDR4 with AIECC
//--------------------------------------------------------------------
CCAErrorResult DDR4e::send(std::bitset<CA_SIZE>& ca, unsigned error) {
    std::bitset<CCA_SIZE> cca;

    // generate extended CA parity
    bool par = tx_wr_toggle;
    for (int i=0; i<PAR_POS; i++) {
        par ^= ca[i];
        cca[i] = ca[i];
    }
    cca[PAR_POS] = par;
    cca[ODT_POS] = 0;   // low
    cca[CS_POS]  = 0;   // select
    cca[CKE_POS] = 1;   // clock enable

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        tx_wr_toggle = !tx_wr_toggle;
    }

    // inject errors
    for (int i=0; i<cca.size(); i++) {
        cca[i] = cca[i] ^ ((error>>i)&1);
    }

    // receive
    return receive(cca);
}

//--------------------------------------------------------------------
// receive interface
//--------------------------------------------------------------------
CCAErrorResult DDR4e::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        } else if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        } else if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e1::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e1::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e2::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    //if (par) {
    //    return CCA_DE_PAR;
    //}

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else 
        if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        //} else if (ccaErrorInfo[type].eDECCdetect) {
        //    if (ccaErrorInfo[type].isMDC) {
        //        return CCA_DE_DECC_LATE;
        //    } else {
        //        return CCA_DE_DECC;
        //    }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e2::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else
        if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        //} else if (ccaErrorInfo[type].eDECCdetect) {
        //    if (ccaErrorInfo[type].isMDC) {
        //        return CCA_DE_DECC_LATE;
        //    } else {
        //        return CCA_DE_DECC;
        //    }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e3::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    //if (par) {
    //    return CCA_DE_PAR;
    //}

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else if (ccaErrorInfo[type].eWCRCdetect) {
        //    return CCA_DE_WCRC;
        //} else
        if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e3::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else if (ccaErrorInfo[type].eWCRCdetect) {
        //    return CCA_DE_WCRC;
        //} else
        if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e4::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    //if (par) {
    //    return CCA_DE_PAR;
    //}

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        //} else if (ccaErrorInfo[type].eWCRCdetect) {
        //    return CCA_DE_WCRC;
        //} else if (ccaErrorInfo[type].eDECCdetect) {
        //    if (ccaErrorInfo[type].isMDC) {
        //        return CCA_DE_DECC_LATE;
        //    } else {
        //        return CCA_DE_DECC;
        //    }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e4::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        //} else if (ccaErrorInfo[type].eWCRCdetect) {
        //    return CCA_DE_WCRC;
        //} else if (ccaErrorInfo[type].eDECCdetect) {
        //    if (ccaErrorInfo[type].isMDC) {
        //        return CCA_DE_DECC_LATE;
        //    } else {
        //        return CCA_DE_DECC;
        //    }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e5::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    //if (par) {
    //    return CCA_DE_PAR;
    //}

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else
        if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e5::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else
        if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e6::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        //} else if (ccaErrorInfo[type].eWCRCdetect) {
        //    return CCA_DE_WCRC;
        //} else if (ccaErrorInfo[type].eDECCdetect) {
        //    if (ccaErrorInfo[type].isMDC) {
        //        return CCA_DE_DECC_LATE;
        //    } else {
        //        return CCA_DE_DECC;
        //    }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e6::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        //} else if (ccaErrorInfo[type].eWCRCdetect) {
        //    return CCA_DE_WCRC;
        //} else if (ccaErrorInfo[type].eDECCdetect) {
        //    if (ccaErrorInfo[type].isMDC) {
        //        return CCA_DE_DECC_LATE;
        //    } else {
        //        return CCA_DE_DECC;
        //    }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e7::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else
        if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e7::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        //if (ccaErrorInfo[type].FSMdetect) {
        //    return CCA_DE_FSM;
        //} else
        if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

//--------------------------------------------------------------------
CCAErrorResult DDR4e8::receive(std::bitset<CCA_SIZE>& cca) {
    // check extended CA parity
    bool par = rx_wr_toggle;
    for (int i=0; i<=PAR_POS; i++) {
        par ^= cca[i];
    }
    if (par) {
        return CCA_DE_PAR;
    }

    // update toggle
    if (cca[CKE_POS] & !cca[CS_POS] & cca[ACT_POS] & cca[RAS_POS] & !cca[CAS_POS] & !cca[WE_POS]) {
        DBG_DALE(printf("RX toggle %d\n", rx_wr_toggle);)
        rx_wr_toggle = !rx_wr_toggle;
    } else {
        DBG_DALE(printf("RX        %d\n", rx_wr_toggle);)
    }

    CCAErrorType type = receive_internal(cca);

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        } else if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

CCAErrorResult DDR4e8::check() {
    CCAErrorType type = sb_check();

    if (ccaErrorInfo[type].isError) {
        if (ccaErrorInfo[type].FSMdetect) {
            return CCA_DE_FSM;
        } else if (ccaErrorInfo[type].eWCRCdetect) {
            return CCA_DE_WCRC;
        } else if (ccaErrorInfo[type].eDECCdetect) {
            if (ccaErrorInfo[type].isMDC) {
                return CCA_DE_DECC_LATE;
            } else {
                return CCA_DE_DECC;
            }
        } else if (ccaErrorInfo[type].isMDC) {
            return CCA_SMDC;
        } else {
            return CCA_SDC;
        }
    } else {
        return CCA_NE;
    }
}

