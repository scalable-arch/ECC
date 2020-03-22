#include <assert.h>

#include "FECC.hh"
#include "FaultDomain.hh"
#include "DomainGroup.hh"
#include "codec.hh"
#include "hsiao.hh"
#include "rs.hh"

//------------------------------------------------------------------------------
bool FrugalECC::isHammingDistance(int origValue, int newValue, int distance) {
    int temp, cnt;
    temp = origValue ^ newValue;
    for (int i=0; i<9; i++) {
        cnt += ((temp>>i)&1);
    }
    return (cnt<=distance);
}

//------------------------------------------------------------------------------
FrugalECC64b::FrugalECC64b() : FrugalECC() {
    layout = PIN;
    codec64 = new RS<2, 8>("F-ECC 64\t", 64, 8, 4);
    codec642 = new RS<2, 8>("F-ECC 64'\t", 64, 4, 0);
    codec68 = new RS<2, 8>("F-ECC 68\t", 68, 8, 4);
    codec73 = new RS<2, 8>("F-ECC 73\t", 73, 8, 4);
}

FrugalECC64b::~FrugalECC64b() {
    delete codec64;
    delete codec642;
    delete codec68;
    delete codec73;
}

bool FrugalECC64b::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(32)<<4)
                 |(errorBlk.getBit(24)<<3)
                 |(errorBlk.getBit(16)<<2)
                 |(errorBlk.getBit( 8)<<1)
                 |(errorBlk.getBit( 0)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 0, 0, 0, 0, x
        origEF = 0 | errorBlk.getBit(0);
    } else if (randValue < 0.7) {   // 1. fully compressed
        origCase = 1;
        // 1, 1, 1, 0, 0
        origEF = 0x1c;
    } else {                        // 2. half compressed
        origCase = 2;
        // 0, 1, 1, 1, 1
        origEF = 0xF;
    }

    newEF = origEF ^ errorEF;

    if (isHammingDistance(newEF, 0x0, 1)) {
        newCase = 0;
    }
    if (isHammingDistance(newEF, 0x1c, 1)) {
        assert(newCase==-1);
        newCase = 1;
    }
    if (isHammingDistance(newEF, 0xF, 1)) {
        assert(newCase==-1);
        newCase = 2;
    }
    if (newCase==-1) {
        return false;
    }
    return true;
}

ErrorType FrugalECC64b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    int origCase, newCase = -1;

    if (!decodeEF(errorBlk, origCase, newCase)) {
        return DUE;
    }

    ErrorType result;
    if (newCase==0) {           // 0. uncompressed
        // : 73B = 64B + 9B
        ECCWord msg = {73*8, 64*8};
        ECCWord decoded = {73*8, 64*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase==0) {
            // fill 64~72 bytes with zeros.
            for (int i=64; i<73; i++) {
                msg.setSymbol(8, i, 0);
            }
        } else {
            // fill 64~72 bytes with random data.
            for (int i=64; i<73; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec73->decode(&msg, &decoded, &correctedPosSet);
    } else if (newCase==1) {    // 1. fully compressed
        // : 64B = 56B + 8B
        ECCWord msg = {64*8, 56*8};
        ECCWord decoded = {64*8, 56*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase!=1) {
            // fill 56~63 bytes (ECC) with random data.
            for (int i=56; i<64; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec64->decode(&msg, &decoded, &correctedPosSet);
    } else {
        // : 64B = 60B + 4B
        ECCWord msg = {64*8, 60*8};
        ECCWord decoded = {64*8, 60*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase==0) {
            // fill 60~63 bytes (ECC) with random data.
            for (int i=60; i<64; i++) {
                msg.invSymbol(8, i, rand()%0x100);                    
            }
        }

        // decode
        result = codec642->decode(&msg, &decoded, &correctedPosSet);
        if (result==DUE) {
            ECCWord msg2 = {68*8, 60*8};
            ECCWord decoded2 = {68*8, 60*8};

            // copy 0~71 bytes from dirty block
            msg2.extract(&errorBlk, layout, 0, 64);

            if (origCase==0) {
                // fill 60~63 bytes (ECC) with the previous random data.
                for (int i=60; i<64; i++) {
                    msg2.invSymbol(8, i, msg.getSymbol(8, i));                        
                }
            }
            if (origCase!=2) {
                // fill 64~68 bytes (ECC) with random data.
                for (int i=64; i<68; i++) {
                    msg2.setSymbol(8, i, rand()%0x100);
                }
            }

            result = codec68->decode(&msg2, &decoded2, &correctedPosSet);
        }
        assert(newCase==2);
    }

    assert(result!=NE);

    return result;
}

ErrorType FrugalECC64b::postprocess(FaultDomain *fd, ErrorType preResult) {
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
bool FrugalECC64bNoEFP::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(8)<<1)
                 |(errorBlk.getBit(0)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 0, x
        origEF = 0 | errorBlk.getBit(0);
    } else if (randValue < 0.7) {   // 1. fully compressed
        origCase = 1;
        // 1, 0
        origEF = 0x2;
    } else {                        // 2. half compressed
        origCase = 2;
        // 1, 1
        origEF = 0x3;
    }

    newEF = origEF ^ errorEF;

    if (newEF==0x2) {
        newCase = 1;
    } else if (newEF==0x03) {
        newCase = 2;
    } else {
        newCase = 0;
    }

    return true;
}

//------------------------------------------------------------------------------
FrugalECC64b2::FrugalECC64b2() : FrugalECC() {
    layout = PIN;
    codec64 = new RS<2, 8>("F-ECC 64\t", 64, 4, 1);
    codec69 = new RS<2, 8>("F-ECC 69\t", 69, 4, 1);
}

FrugalECC64b2::~FrugalECC64b2() {
    delete codec64;
    delete codec69;
}

bool FrugalECC64b2::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(16)<<2)
                 |(errorBlk.getBit(8)<<1)
                 |(errorBlk.getBit(0)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 0, 0, 0
        origEF = 0x0;
    } else {                        // 1. compressed
        origCase = 1;
        // 1, 1, 1
        origEF = 0x7;
    }

    newEF = origEF ^ errorEF;

    if (isHammingDistance(newEF, 0x0, 1)) {
        newCase = 0;
    }
    if (isHammingDistance(newEF, 0x7, 1)) {
        assert(newCase==-1);
        newCase = 1;
    }
    if (newCase==-1) {
        return false;
    }
    return true;
}

ErrorType FrugalECC64b2::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    int origCase, newCase = -1;

    if (!decodeEF(errorBlk, origCase, newCase)) {
        return DUE;
    }

    ErrorType result;
    if (newCase==0) {           // 0. uncompressed
        // : 73B = 64B + 9B
        ECCWord msg = {69*8, 64*8};
        ECCWord decoded = {69*8, 64*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase==0) {
            // fill 64~72 bytes with zeros.
            for (int i=64; i<69; i++) {
                msg.setSymbol(8, i, 0);
            }
        } else {
            // fill 64~72 bytes with random data.
            for (int i=64; i<69; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec69->decode(&msg, &decoded, &correctedPosSet);
    } else {    // 1. fully compressed
        // : 64B = 56B + 8B
        ECCWord msg = {64*8, 60*8};
        ECCWord decoded = {64*8, 60*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase!=1) {
            // fill 56~63 bytes (ECC) with random data.
            for (int i=60; i<64; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec64->decode(&msg, &decoded, &correctedPosSet);
    }

    assert(result!=NE);

    return result;
}

//------------------------------------------------------------------------------
FrugalECC64b3::FrugalECC64b3() : FrugalECC() {
    layout = PIN;
    codec64 = new RS<2, 8>("F-ECC 64\t", 64, 16, 8);
    codec642 = new RS<2, 8>("F-ECC 64'\t", 64, 8, 0);
    codec72 = new RS<2, 8>("F-ECC 72\t", 72, 16, 8);
    codec81 = new RS<2, 8>("F-ECC 81\t", 81, 16, 8);
}

FrugalECC64b3::~FrugalECC64b3() {
    delete codec64;
    delete codec642;
    delete codec72;
    delete codec81;
}

bool FrugalECC64b3::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(32)<<8)
                 |(errorBlk.getBit(28)<<7)
                 |(errorBlk.getBit(24)<<6)
                 |(errorBlk.getBit(20)<<5)
                 |(errorBlk.getBit(16)<<4)
                 |(errorBlk.getBit(12)<<3)
                 |(errorBlk.getBit( 8)<<2)
                 |(errorBlk.getBit( 4)<<1)
                 |(errorBlk.getBit( 0)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 0, 0, 0, 0, 0, 0, x
        origEF = 0 | errorBlk.getBit(0);
    } else if (randValue < 0.7) {   // 1. fully compressed
        origCase = 1;
        // 1, 1, 1, 1, 1, 0, 0, 0, 0: HD=5 from others
        origEF = 0x1F0;
    } else {                        // 2. half compressed
        origCase = 2;
        // 0, 0, 0, 1, 1, 1, 1, 1
        origEF = 0x1F;
    }

    newEF = origEF ^ errorEF;

    if (isHammingDistance(newEF, 0x0, 2)) {
        newCase = 0;
    }
    if (isHammingDistance(newEF, 0x1F0, 2)) {
        assert(newCase==-1);
        newCase = 1;
    }
    if (isHammingDistance(newEF, 0x1F, 2)) {
        assert(newCase==-1);
        newCase = 2;
    }
    if (newCase==-1) {
        return false;
    }
    return true;
}

ErrorType FrugalECC64b3::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    int origCase, newCase = -1;

    if (!decodeEF(errorBlk, origCase, newCase)) {
        return DUE;
    }

    ErrorType result;
    if (newCase==0) {           // 0. uncompressed
        // : 81B = 64B + 17B
        ECCWord msg = {81*8, 64*8};
        ECCWord decoded = {81*8, 64*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase==0) {
            // fill 64~80 bytes with zeros.
            for (int i=64; i<81; i++) {
                msg.setSymbol(8, i, 0);
            }
        } else {
            // fill 64~80 bytes with random data.
            for (int i=64; i<81; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec81->decode(&msg, &decoded, &correctedPosSet);
    } else if (newCase==1) {    // 1. fully compressed
        // : 64B = 48B + 16B
        ECCWord msg = {64*8, 48*8};
        ECCWord decoded = {64*8, 48*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase!=1) {
            // fill 48~63 bytes (ECC) with random data.
            for (int i=48; i<64; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec64->decode(&msg, &decoded, &correctedPosSet);
    } else {
        // : 64B = 56B + 8B
        ECCWord msg = {64*8, 56*8};
        ECCWord decoded = {64*8, 56*8};

        // copy 0~63 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 64);

        if (origCase==0) {
            // fill 60~63 bytes (ECC) with random data.
            for (int i=56; i<64; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec642->decode(&msg, &decoded, &correctedPosSet);
        if (result==DUE) {
            ECCWord msg2 = {72*8, 56*8};
            ECCWord decoded2 = {72*8, 56*8};

            // copy 0~63 bytes from dirty block
            msg2.extract(&errorBlk, layout, 0, 64);

            if (origCase==0) {
                // fill 56~63 bytes (ECC) with the previous random data.
                for (int i=56; i<64; i++) {
                    msg2.invSymbol(8, i, msg.getSymbol(8, i));
                }
            }
            if (origCase!=2) {
                // fill 64~71 bytes (ECC) with random data.
                for (int i=64; i<72; i++) {
                    msg2.setSymbol(8, i, rand()%0x100);
                }
            }

            result = codec72->decode(&msg2, &decoded2, &correctedPosSet);
        }
        assert(newCase==2);
    }

    assert(result!=NE);

    return result;
}

//------------------------------------------------------------------------------
FrugalECC72bOPC::FrugalECC72bOPC() : FrugalECC() {
    layout = PIN;
    codec72 = new RS<2, 8>("F-ECC 72\t", 72, 16, 8);
    codec722 = new RS<2, 8>("F-ECC 72'\t", 72, 8, 0);
    codec80 = new RS<2, 8>("F-ECC 80\t", 80, 16, 8);
    codec81 = new RS<2, 8>("F-ECC 81\t", 81, 16, 8);
}

FrugalECC72bOPC::~FrugalECC72bOPC() {
    delete codec72;
    delete codec722;
    delete codec80;
    delete codec81;
}

bool FrugalECC72bOPC::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(32)<<4)
                 |(errorBlk.getBit(24)<<3)
                 |(errorBlk.getBit(16)<<2)
                 |(errorBlk.getBit( 8)<<1)
                 |(errorBlk.getBit( 0)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 0, 0, 0, 0, x
        origEF = 0 | errorBlk.getBit(0);
    } else if (randValue < 0.7) {   // 1. fully compressed
        origCase = 1;
        // 1, 1, 1, 0, 0
        origEF = 0x1c;
    } else {                        // 2. half compressed
        origCase = 2;
        // 0, 1, 1, 1, 1
        origEF = 0xF;
    }

    newEF = origEF ^ errorEF;

    if (isHammingDistance(newEF, 0x0, 1)) {
        newCase = 0;
    }
    if (isHammingDistance(newEF, 0x1c, 1)) {
        assert(newCase==-1);
        newCase = 1;
    }
    if (isHammingDistance(newEF, 0xF, 1)) {
        assert(newCase==-1);
        newCase = 2;
    }
    if (newCase==-1) {
        return false;
    }
    return true;
}

ErrorType FrugalECC72bOPC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    int origCase, newCase = -1;

    if (!decodeEF(errorBlk, origCase, newCase)) {
        return DUE;
    }

    ErrorType result;
    if (newCase==0) {           // 0. uncompressed
        // : 81B = 72B + 9B = 64B + 16B + 1B
        // 72B + 9B extra
        ECCWord msg = {81*8, 64*8};
        ECCWord decoded = {81*8, 64*8};

        // copy 0~71 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 72);

        if (origCase==0) {
            // fill 72~80 bytes with zeros.
            for (int i=72; i<81; i++) {
                msg.setSymbol(8, i, 0);
            }
        } else {
            // fill 72~80 bytes with random data.
            for (int i=72; i<81; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec81->decode(&msg, &decoded, &correctedPosSet);
    } else if (newCase==1) {    // 1. fully compressed
        // : 72B = 56B + 16B
        ECCWord msg = {72*8, 56*8};
        ECCWord decoded = {72*8, 56*8};

        // copy 0~71 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 72);

        if (origCase!=1) {
            // fill 56~71 bytes (ECC) with random data.
            for (int i=56; i<72; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec72->decode(&msg, &decoded, &correctedPosSet);
    } else {
        // : 72B = 64B + 8B
        ECCWord msg = {72*8, 64*8};
        ECCWord decoded = {72*8, 64*8};

        // copy 0~71 bytes from dirty block
        msg.extract(&errorBlk, layout, 0, 72);

        if (origCase==0) {
            // fill 56~71 bytes (ECC) with random data.
            for (int i=64; i<72; i++) {
                msg.invSymbol(8, i, rand()%0x100);
            }
        }

        // decode
        result = codec722->decode(&msg, &decoded, &correctedPosSet);
        if (result==DUE) {
            ECCWord msg2 = {80*8, 64*8};
            ECCWord decoded2 = {80*8, 64*8};

            // copy 0~71 bytes from dirty block
            msg2.extract(&errorBlk, layout, 0, 72);

            if (origCase==0) {
                // fill 64~72 bytes (ECC) with the previous random data.
                for (int i=64; i<72; i++) {
                    msg2.invSymbol(8, i, msg.getSymbol(8, i));
                }
            }
            if (origCase!=2) {
                // fill 72~80 bytes (ECC) with random data.
                for (int i=72; i<80; i++) {
                    msg2.invSymbol(8, i, rand()%0x100);
                }
            }

            result = codec80->decode(&msg2, &decoded2, &correctedPosSet);
        }
        assert(newCase==2);
    }

    assert(result!=NE);

    return result;
}

ErrorType FrugalECC72bOPC::postprocess(FaultDomain *fd, ErrorType preResult) {
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
bool FrugalECC72bNoEFP::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(8)<<1)
                 |(errorBlk.getBit(0)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 0, x
        origEF = 0 | errorBlk.getBit(0);
    } else if (randValue < 0.7) {   // 1. fully compressed
        origCase = 1;
        // 1, 0
        origEF = 0x2;
    } else {                        // 2. half compressed
        origCase = 2;
        // 1, 1
        origEF = 0x3;
    }

    newEF = origEF ^ errorEF;

    if (newEF==0x2) {
        newCase = 1;
    } else if (newEF==0x03) {
        newCase = 2;
    } else {
        newCase = 0;
    }

    return true;
}

//------------------------------------------------------------------------------
FrugalECC64bMultix8::FrugalECC64bMultix8() : FrugalECC64b() {
    layout = MULTIX8;
//    configList.push_back({0, 0, new RS<2, 16>("S16DC (M-ECC)\t", 8, 1, 0)});
    codec_compressed = new RS<2, 16>("S16DC (M-ECC)\t", 8, 1, 0);
    codec_un_compressed = new RS<2, 16>("S16DC (M-ECC)\t", 10, 1, 0);
}

ErrorType FrugalECC64bMultix8::decodeInternal(FaultDomain *fd, CacheLine &errorBlk)
{
    int origCase, newCase = -1;

    int sym_size = 8;
    if (!decodeEF(errorBlk, origCase, newCase)) {
        return DUE;
    }

    ErrorType result = NE, newResult;

    if (newCase==0) {           // 0. uncompressed
    // decoding here(MultiECC-like)
    // result = codec_un_compressed->decode(&msg, &decoded);
    for (int i=errorBlk.getBeatHeight()/2-1; i>=0; i--) {
        ECCWord msg = {20*sym_size, 18*sym_size};
        ECCWord decoded = {20*sym_size, 18*sym_size};
        msg.reset();
        decoded.reset();

//printf("i: %i getBitN(): %i\tchannel width: %i\n", i, codec_un_compressed->getBitN(),errorBlk.getChannelWidth());
//msg.print();
        
        msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
            
        if (origCase==0) {
            // fill last two chips with zeros.
            for (int i=8; i<10; i++) {
                msg.setSymbol(16, i, 0);
            }
        } else {
            // fill 64~80 bytes with random data.
            for (int i=8; i<10; i++) {
                msg.setSymbol(16, i, rand()%0x10000);
            }
        }

        if (msg.isZero()) {        // error-free region of a block -> skip
            newResult = NE;
        } else {
            newResult = codec_un_compressed->decode(&msg, &decoded);

            if (newResult==DUE) {
                // 1. error is detected by T1EC
                int nonZeroChecksumCount = 0;
                Fault *nonZeroChecksumFR = NULL;

                auto it2 = fd->operationalFaultList.rbegin();
                Fault *newFault = (*it2);
                for (auto it = fd->operationalFaultList.begin(); it != fd->operationalFaultList.end(); it++) {
                    if (!newFault->overlap(*it)) {  // unrelated error
                        continue;
                    }

                    // 2. generate checksum
                    if (!(*it)->getIsSingleDQ()) {
                        uint32_t origChecksum = 0;
                        uint32_t errorChecksum = 0;

                        for (int j=0; j<256; j++) {
                            uint16_t data = rand() % 0x10000;
                            uint16_t error;
                            if (j==0) {
                                error = decoded.getSymbol(16, (*it)->getChipID());
                            } else {
                                if (!(*it)->getIsSingleDQ()) {
                                    error = rand() % 0x10000;
                                } else {
                                    int pinLoc = (*it)->getPinID()%8;
                                    error = ((rand()%2) << pinLoc) | (rand()%2 << (pinLoc+8));
                                }
                            }
                            origChecksum += data;
                            origChecksum = (origChecksum & 0xFFFF) + (origChecksum>>16);
                            errorChecksum += (data ^ error);
                            errorChecksum = (errorChecksum & 0xFFFF) + (errorChecksum>>16);
                        }
                        if (origChecksum!=errorChecksum) {
                            nonZeroChecksumCount++;
                            nonZeroChecksumFR = *it;
                        }
                    } else {
                        nonZeroChecksumCount++;
                        nonZeroChecksumFR = *it;
                    }
                }
                if (nonZeroChecksumCount==1) {
                    // correct it
                    decoded.setSymbol(16, nonZeroChecksumFR->getChipID(), 0);

                    if (decoded.isZero()) {
                        newResult = CE;
                    } else {
                        newResult = SDC;
                    }
                } else {
                    newResult = DUE;
                }
            } else {
                newResult = SDC;
            }
        }

        result = worse2ErrorType(result, newResult);
      }
    } else if (newCase==1) {    // 1. fully compressed
        // : 64B = 56B + 8B
//        ECCWord msg = {16*sym_size, 14*sym_size};
//        ECCWord decoded = {16*sym_size, 14*sym_size};

        // copy 0~63 bytes from dirty block
//        msg.extract(dataPtr, layout, 64);
/*
        if (origCase!=1) {
            // fill 48~63 bytes (ECC) with random data.
            for (int i=56; i<64; i++) {
                msg.setSymbol(8, i, rand()%0x100);
            }
        }
*/
        // decoding here(MultiECC-like)
        //result = codec_compressed->decode(&msg, &decoded);
      for (int i=errorBlk.getBeatHeight()/2-1; i>=0; i--) {
        ECCWord msg = {16*sym_size, 14*sym_size};
        ECCWord decoded = {16*sym_size, 14*sym_size};
        msg.reset();
        decoded.reset();

        msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());

        if (origCase!=1) {
            // fill last chip with random data.
            msg.setSymbol(16, 7, rand()%0x10000);
        }

        if (msg.isZero()) {        // error-free region of a block -> skip
//printf("NE\n");
            newResult = NE;
        } else {
            newResult = codec_compressed->decode(&msg, &decoded);

            if (newResult==DUE) {
                // 1. error is detected by T1EC
                int nonZeroChecksumCount = 0;
                Fault *nonZeroChecksumFR = NULL;

                auto it2 = fd->operationalFaultList.rbegin();
                Fault *newFault = (*it2);
                for (auto it = fd->operationalFaultList.begin(); it != fd->operationalFaultList.end(); it++) {
                    if (!newFault->overlap(*it)) {  // unrelated error
                        continue;
                    }

                    // 2. generate checksum
                    if ((*it)->getIsMultiRow()) {
                        uint32_t origChecksum = 0;
                        uint32_t errorChecksum = 0;

                        for (int j=0; j<256; j++) {
                            uint16_t data = rand() % 0x10000;
                            uint16_t error;
                            if (j==0) {
                                error = decoded.getSymbol(16, (*it)->getChipID());
                            } else {
                                if (!(*it)->getIsSingleDQ()) {
                                    error = rand() % 0x10000;
                                } else {
                                    int pinLoc = (*it)->getPinID()%8;
                                    error = ((rand()%2) << pinLoc) | (rand()%2 << (pinLoc+8));
                                }
                            }
                            origChecksum += data;
                            origChecksum = (origChecksum & 0xFFFF) + (origChecksum>>16);
                            errorChecksum += (data ^ error);
                            errorChecksum = (errorChecksum & 0xFFFF) + (errorChecksum>>16);
                        }
                        if (origChecksum!=errorChecksum) {
                            nonZeroChecksumCount++;
                            nonZeroChecksumFR = *it;
                        }
                    } else {
                        nonZeroChecksumCount++;
                        nonZeroChecksumFR = *it;
                    }
                }
                if (nonZeroChecksumCount==1) {
                    // correct it
                    decoded.setSymbol(16, nonZeroChecksumFR->getChipID(), 0);

                    if (decoded.isZero()) {
//printf("CORRECTED! - compressed\n");
//msg.print();
//                        errorBlk.print();
                        newResult = CE;
                    } else {
//                        printf("SDC from checksum error - compressed\n");
//                        fd->print();
//                        decoded.print();
//                        errorBlk.print();
                        newResult = SDC;
                    }
                } else {
//												printf("nonZeroChecksumCount %i\n", nonZeroChecksumCount);
//                        fd->print();
//                        decoded.print();
//                        errorBlk.print();
//												assert(0);
                    newResult = DUE;
                }
            } else {
//                printf("SDC from RS error\n");
//                fd->print();
//                decoded.print();
//                errorBlk.print();
                newResult = SDC;
            }
        }

        result = worse2ErrorType(result, newResult);
      }
    }
    // 2. half compressed possible? NO

//    assert(result!=NE);

    return result;

}


bool FrugalECC64bMultix8::decodeEF(CacheLine &errorBlk, int& origCase, int& newCase) {
    int origEF, newEF;

    int errorEF = (errorBlk.getBit(32)<<4)
                 |(errorBlk.getBit(24)<<3)
                 |(errorBlk.getBit(24)<<2)
                 |(errorBlk.getBit(24)<<1)
                 |(errorBlk.getBit(24)<<0);

    double randValue = drand48();
    if (randValue > 0.9) {          // 0. uncompressed
        origCase = 0;
        // 1, 1, 1, 0, 0
        origEF = 0x1c;
    } else if (randValue <= 0.9) {   // 1. fully compressed
        origCase = 1;
        // 0, 0, 0, 0, x
        origEF = 0 | errorBlk.getBit(0);
    }
    /*else {                        // 2. half compressed
        origCase = 2;
        // 0, 1, 1, 1, 1
        origEF = 0xF;
    }
*/
    newEF = origEF ^ errorEF;

    if (isHammingDistance(newEF, 0x1c, 1)) {
        newCase = 0;
    }
    else{
        newCase = 1;
    }
/*    if (isHammingDistance(newEF, 0x0, 1)) {
        assert(newCase==-1);
        newCase = 1;
    }
    if (isHammingDistance(newEF, 0xF, 1)) {
        assert(newCase==-1);
        newCase = 2;
    }
    */
    if (newCase==-1) {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
FrugalECC64bMultix4::FrugalECC64bMultix4() : FrugalECC64bMultix8() {
    layout = MULTIX4;
    codec_compressed = new RS<2, 16>("S16DC (M-ECC)\t", 16, 1, 0);
    codec_un_compressed = new RS<2, 16>("S16DC (M-ECC)\t", 18, 1, 0);
}

ErrorType FrugalECC64bMultix4::decodeInternal(FaultDomain *fd, CacheLine &errorBlk)
{
    int origCase, newCase = -1;

    int sym_size = 16;
    if (!decodeEF(errorBlk, origCase, newCase)) {
        return DUE;
    }

    ErrorType result = NE, newResult;

    if (newCase==0) {           // 0. uncompressed
    // decoding here(MultiECC-like)
    // result = codec_un_compressed->decode(&msg, &decoded);
    for (int i=errorBlk.getBeatHeight()/4-1; i>=0; i--) {
        ECCWord msg = {18*sym_size, 17*sym_size};
        ECCWord decoded = {18*sym_size, 17*sym_size};
        msg.reset();
        decoded.reset();

        msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
        
        if (origCase==0) {
            // fill last two chips with zeros.
            for (int i=16; i<18; i++) {
                msg.setSymbol(16, i, 0);
            }
        } else {
            // fill 64~80 bytes with random data.
            for (int i=16; i<18; i++) {
                msg.setSymbol(16, i, rand()%0x10000);
            }
        }

        if (msg.isZero()) {        // error-free region of a block -> skip
            newResult = NE;
        } else {
            newResult = codec_un_compressed->decode(&msg, &decoded);

            if (newResult==DUE) {
                // 1. error is detected by T1EC
                int nonZeroChecksumCount = 0;
                Fault *nonZeroChecksumFR = NULL;

                auto it2 = fd->operationalFaultList.rbegin();
                Fault *newFault = (*it2);
                for (auto it = fd->operationalFaultList.begin(); it != fd->operationalFaultList.end(); it++) {
                    if (!newFault->overlap(*it)) {  // unrelated error
                        continue;
                    }

                    // 2. generate checksum
                    if ((*it)->getIsMultiRow()) {
                        uint32_t origChecksum = 0;
                        uint32_t errorChecksum = 0;

                        for (int j=0; j<256; j++) {
                            uint16_t data = rand() % 0x10000;
                            uint16_t error;
                            if (j==0) {
                                error = decoded.getSymbol(16, (*it)->getChipID());
                            } else {
                                if (!(*it)->getIsSingleDQ()) {
                                    error = rand() % 0x10000;
                                } else {
                                    int pinLoc = (*it)->getPinID()%8;
                                    error = ((rand()%2) << pinLoc) | (rand()%2 << (pinLoc+8));
                                }
                            }
                            origChecksum += data;
                            origChecksum = (origChecksum & 0xFFFF) + (origChecksum>>16);
                            errorChecksum += (data ^ error);
                            errorChecksum = (errorChecksum & 0xFFFF) + (errorChecksum>>16);
                        }
                        if (origChecksum!=errorChecksum) {
                            nonZeroChecksumCount++;
                            nonZeroChecksumFR = *it;
                        }
                    } else {
                        nonZeroChecksumCount++;
                        nonZeroChecksumFR = *it;
                    }
                }
                if (nonZeroChecksumCount==1) {
                    // correct it
                    decoded.setSymbol(16, nonZeroChecksumFR->getChipID(), 0);

                    if (decoded.isZero()) {
                        newResult = CE;
                    } else {
                        newResult = SDC;
                    }
                } else {
                    newResult = DUE;
                }
            } else {
                newResult = SDC;
            }
        }

        result = worse2ErrorType(result, newResult);
      }
    } else if (newCase==1) {    // 1. fully compressed
        // : 64B = 56B + 8B
//        ECCWord msg = {16*sym_size, 14*sym_size};
//        ECCWord decoded = {16*sym_size, 14*sym_size};

        // copy 0~63 bytes from dirty block
//        msg.extract(dataPtr, layout, 64);
        // decoding here(MultiECC-like)
        //result = codec_compressed->decode(&msg, &decoded);
        for (int i=errorBlk.getBeatHeight()/4-1; i>=0; i--) {
            ECCWord msg = {16*sym_size, 15*sym_size};
            ECCWord decoded = {16*sym_size, 15*sym_size};
            msg.reset();
            decoded.reset();
    
            msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
    
            if (origCase!=1) {
                // fill last chip with random data.
                msg.setSymbol(16, 15, rand()%0x10000);
            }
    
            if (msg.isZero()) {        // error-free region of a block -> skip
                newResult = NE;
            } else {
                newResult = codec_compressed->decode(&msg, &decoded);
    
                if (newResult==DUE) {
                    // 1. error is detected by T1EC
                    int nonZeroChecksumCount = 0;
                    Fault *nonZeroChecksumFR = NULL;
    
                    auto it2 = fd->operationalFaultList.rbegin();
                    Fault *newFault = (*it2);
                    for (auto it = fd->operationalFaultList.begin(); it != fd->operationalFaultList.end(); it++) {
                        if (!newFault->overlap(*it)) {  // unrelated error
                            continue;
                        }
    
                        // 2. generate checksum
                        if ((*it)->getIsMultiRow()) {
                            uint32_t origChecksum = 0;
                            uint32_t errorChecksum = 0;
    
                            for (int j=0; j<256; j++) {
                                uint16_t data = rand() % 0x10000;
                                uint16_t error;
                                if (j==0) {
                                    error = decoded.getSymbol(16, (*it)->getChipID());
                                } else {
                                    if (!(*it)->getIsSingleDQ()) {
                                        error = rand() % 0x10000;
                                    } else {
                                        int pinLoc = (*it)->getPinID()%8;
                                        error = ((rand()%2) << pinLoc) | (rand()%2 << (pinLoc+8));
                                    }
                                }
                                origChecksum += data;
                                origChecksum = (origChecksum & 0xFFFF) + (origChecksum>>16);
                                errorChecksum += (data ^ error);
                                errorChecksum = (errorChecksum & 0xFFFF) + (errorChecksum>>16);
                            }
                            if (origChecksum!=errorChecksum) {
                                nonZeroChecksumCount++;
                                nonZeroChecksumFR = *it;
                            }
                        } else {
                            nonZeroChecksumCount++;
                            nonZeroChecksumFR = *it;
                        }
                    }
                    if (nonZeroChecksumCount==1) {
                        // correct it
                        decoded.setSymbol(16, nonZeroChecksumFR->getChipID(), 0);
    
                        if (decoded.isZero()) {
                            newResult = CE;
                        } else {
                            newResult = SDC;
                        }
                    } else {
                        newResult = DUE;
                    }
                } else {
                    newResult = SDC;
                }
            }
    
            result = worse2ErrorType(result, newResult);
        }
    } 
    // 2. half compressed possible? NO

//    assert(result!=NE);

    return result;

}

