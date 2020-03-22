#include <assert.h>

#include "AIECC.hh"
#include "FaultDomain.hh"
#include "DomainGroup.hh"
#include "codec.hh"
#include "hsiao.hh"
#include "rs.hh"

#ifdef AIECC
//------------------------------------------------------------------------------
ErrorType CCAECC::decode(FaultDomain *fd, CacheLine &errorBlk) {
    if (errorBlk.hasCAError()) {
        CCAErrorResult result[4];
        CCAErrorResult post_result;

        cur_cycle = 0;
        dram->init();

        // open all banks
        for (int row_bid=0; row_bid<16; row_bid++) {
            int bg = row_bid & 3;
            int ba = row_bid >> 2;
            dram->activate(bg, ba, ROW_ADDR+(bg<<2)+ba, 0);
            cur_cycle += 4;
        }

        cur_cycle += 20;
        result[0] = dram->precharge(0, 0, errorBlk.getPreError());
        if (result[0]!=CCA_NE) {
            if ((result[0]==CCA_SDC)||(result[0]==CCA_SMDC)) {
                return SDC;
            } else {
                return DUE;
            }
        }

        cur_cycle += 30;
        result[1] = dram->activate(0, 0, ROW_ADDR+(0<<2)+0, errorBlk.getActError());
        if (result[1]!=CCA_NE) {
            if ((result[1]==CCA_SDC)||(result[1]==CCA_SMDC)) {
                return SDC;
            } else {
                return DUE;
            }
        }

        cur_cycle += 30;
        result[2] = dram->wr_bl8(0, 0, WR_ADDR+(0<<2)+0, errorBlk.getWrError());
        if (result[2]!=CCA_NE) {
            if ((result[2]==CCA_SDC)||(result[2]==CCA_SMDC)) {
                return SDC;
            } else {
                return DUE;
            }
        }

        cur_cycle += 30;
        result[3] = dram->rd_bl8(0, 0, RD_ADDR+(0<<2)+0, errorBlk.getRdError());
        if (result[3]!=CCA_NE) {
            if ((result[3]==CCA_SDC)||(result[3]==CCA_SMDC)) {
                return SDC;
            } else {
                return DUE;
            }
        }

        cur_cycle += 30;

        // 1. check scoreboard
        post_result = dram->check();
        if (post_result!=CCA_NE) {
            if ((post_result==CCA_SDC)||(post_result==CCA_SMDC)) {
                return SDC;
            } else {
                return DUE;
            }
        }

        // 2. read all banks
        post_result = CCA_NE;
        for (int row_bid=0; row_bid<16; row_bid++) {
            int bg = row_bid & 3;
            int ba = row_bid >> 2;
            CCAErrorResult result = dram->rd_bl8(bg, ba, RD_ADDR+(bg<<2)+ba, 0);
            cur_cycle += 4;
            if (result!=CCA_NE) {
                post_result = result;
                if ((post_result==CCA_SDC)||(post_result==CCA_SMDC)) {
                    return SDC;
                } else {
                    return DUE;
                }
            }
        }
        return NE;
    }
    return dataECC->decode(fd, errorBlk);
}

//------------------------------------------------------------------------------
ErrorType DDR4QPC72b::decode(FaultDomain *fd, CacheLine &errorBlk) {
    // find appropriate CODEC
    Codec *codec = NULL;
    for (auto it = dataECC->configList.begin(); it != dataECC->configList.end(); it++) {
        if (   (fd->getRetiredChipCount() <= it->maxDeviceRetirement)
            && (fd->getRetiredPinCount() <= it->maxPinRetirement) ) {
            codec = it->codec;
        }
    }
    if (codec==NULL) { return SDC; }

    preprocess(fd);

    Message msg = {codec->getBitN(), codec->getBitK()};
    Message decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType result = NE;
    msg.extract(errorBlk, PIN, 0, errorBlk.getChannelWidth());
    if (errorBlk.hasAddrError()) {
        return SDC;
    } else {
        if (msg.isZero()) {        // error-free region of a block -> skip
            result = NE;
        } else {
            result = codec->decode(&msg, &decoded, &correctedPosSet);

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
        }
    }

    assert(result!=NE || errorBlk.hasCAError());

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

    return result;
}

//------------------------------------------------------------------------------
ErrorType AzulQPC72b::decode(FaultDomain *fd, CacheLine &errorBlk) {
    // find appropriate CODEC
    Codec *codec = NULL;
    for (auto it = dataECC->configList.begin(); it != dataECC->configList.end(); it++) {
        if (   (fd->getRetiredChipCount() <= it->maxDeviceRetirement)
            && (fd->getRetiredPinCount() <= it->maxPinRetirement) ) {
            codec = it->codec;
        }
    }
    if (codec==NULL) { return SDC; }

    preprocess(fd);

    Message msg = {codec->getBitN(), codec->getBitK()};
    Message decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType result = NE;
    msg.extract(errorBlk, PIN, 0, errorBlk.getChannelWidth());
    if (errorBlk.hasAddrError()) {
        int error_lsb = -1;
        int error_msb = -1;
        for (int i=0; i<32; i++) {
            if ((errorBlk.getAddrError()>>i) & 1) {
                error_lsb = i;
                break;
            }
        }
        for (int i=31; i>=0; i--) {
            if ((errorBlk.getAddrError()>>i) & 1) {
                error_msb = i;
                break;
            }
        }
        int burst_length = error_msb - error_lsb;
        unsigned checksum;
        if (burst_length<4) {   // always non-zero checksum
            checksum = 1+rand()%15;
        } else {
            checksum = rand()%16;
        }

        xorSym(msg.getDataPtr(), 1,  0, (checksum>>0)&1);
        xorSym(msg.getDataPtr(), 1,  8, (checksum>>1)&1);
        xorSym(msg.getDataPtr(), 1, 16, (checksum>>2)&1);
        xorSym(msg.getDataPtr(), 1, 24, (checksum>>3)&1);

        xorSym(msg.getDataPtr(), 1, 32, (checksum>>0)&1);
        xorSym(msg.getDataPtr(), 1, 40, (checksum>>1)&1);
        xorSym(msg.getDataPtr(), 1, 48, (checksum>>2)&1);
        xorSym(msg.getDataPtr(), 1, 56, (checksum>>3)&1);

        xorSym(msg.getDataPtr(), 1, 64, (checksum>>0)&1);
        xorSym(msg.getDataPtr(), 1, 72, (checksum>>1)&1);
        xorSym(msg.getDataPtr(), 1, 80, (checksum>>2)&1);
        xorSym(msg.getDataPtr(), 1, 88, (checksum>>3)&1);

        //xorSym(msg.getDataPtr(), 1, 96, (checksum>>0)&1);
        //xorSym(msg.getDataPtr(), 1, 104, (checksum>>1)&1);
        //xorSym(msg.getDataPtr(), 1, 112, (checksum>>2)&1);
        //xorSym(msg.getDataPtr(), 1, 120, (checksum>>3)&1);
        //xorSym(msg.getDataPtr(), 1, 128, (checksum>>0)&1);
        //xorSym(msg.getDataPtr(), 1, 136, (checksum>>1)&1);
        //xorSym(msg.getDataPtr(), 1, 144, (checksum>>2)&1);
        //xorSym(msg.getDataPtr(), 1, 152, (checksum>>3)&1);

        if (msg.isZero()) {
            return SDC;
        }
    } else {
        if (msg.isZero()) {        // error-free region of a block -> skip
            assert(0);
        }
    }

    result = codec->decode(&msg, &decoded, &correctedPosSet);

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

    return result;
}

//------------------------------------------------------------------------------
ErrorType NickQPC72b::decode(FaultDomain *fd, CacheLine &errorBlk) {
    // find appropriate CODEC
    Codec *codec = NULL;
    for (auto it = dataECC->configList.begin(); it != dataECC->configList.end(); it++) {
        if (   (fd->getRetiredChipCount() <= it->maxDeviceRetirement)
            && (fd->getRetiredPinCount() <= it->maxPinRetirement) ) {
            codec = it->codec;
        }
    }
    if (codec==NULL) { return SDC; }

    preprocess(fd);

    Message msg = {codec->getBitN(), codec->getBitK()};
    Message decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType result = NE;
    msg.extract(errorBlk, PIN, 0, errorBlk.getChannelWidth());
    if (errorBlk.hasAddrError()) {
        for (int addrPos = 0; addrPos<32; addrPos++) {
            if ((errorBlk.getAddrError()>>addrPos) & 1) {
                int beatPos = addrPos/4;
                int pinPos = addrPos%4;
                for (int chipPos = 0; chipPos<16; chipPos++) {  // data chips only
                    xorSym(msg.getDataPtr(), 1, (pinPos + 4*chipPos)*8 + beatPos, 1);
                }

            }
        }

        if (msg.isZero()) {
            return SDC;
        }
    } else {
        if (msg.isZero()) {        // error-free region of a block -> skip
            assert(0);
        }
    }

    result = codec->decode(&msg, &decoded, &correctedPosSet);

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

    return result;
}

//------------------------------------------------------------------------------
ErrorType AIECCQPC72b::decode(FaultDomain *fd, CacheLine &errorBlk) {
    // find appropriate CODEC
    Codec *codec = NULL;
    for (auto it = dataECC->configList.begin(); it != dataECC->configList.end(); it++) {
        if (   (fd->getRetiredChipCount() <= it->maxDeviceRetirement)
            && (fd->getRetiredPinCount() <= it->maxPinRetirement) ) {
            codec = it->codec;
        }
    }
    if (codec==NULL) { return SDC; }

    preprocess(fd);

    Message msg = {codec->getBitN(), codec->getBitK()};
    Message decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType result = NE;
    msg.extract(errorBlk, PIN, 0, errorBlk.getChannelWidth());
    if (errorBlk.hasAddrError()) {
        msg.setSymbol(8, 72, (errorBlk.getAddrError()>> 0)&0xFF);
        msg.setSymbol(8, 73, (errorBlk.getAddrError()>> 8)&0xFF);
        msg.setSymbol(8, 74, (errorBlk.getAddrError()>>16)&0xFF);
        msg.setSymbol(8, 75, (errorBlk.getAddrError()>>24)&0xFF);

        if (msg.isZero()) {
            assert(0);
        }
    } else {
        if (msg.isZero()) {        // error-free region of a block -> skip
            assert(0);
        }
    }

    result = codec->decode(&msg, &decoded, &correctedPosSet);

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

    return result;
}

#endif
//------------------------------------------------------------------------------
