#include "MultiECC.hh"
#include "rs.hh"

//------------------------------------------------------------------------------
// “Low-power, low-storage-overhead chipkill correct via multi-line error correction"
//------------------------------------------------------------------------------
MultiECC::MultiECC() : ECC(MULTIX8) {
    configList.push_back({0, 0, new RS<2, 16>("S16DC (M-ECC)\t", 9, 1, 0)});
}

ErrorType MultiECC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    Codec *codec = configList.front().codec;

    ECCWord msg = {codec->getBitN(), codec->getBitK()};
    ECCWord decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType result = NE, newResult; 
    for (int i=errorBlk.getBeatHeight()/2-1; i>=0; i--) {
        msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());

        if (msg.isZero()) {        // error-free region of a block -> skip
            newResult = NE;
        } else {
            newResult = codec->decode(&msg, &decoded);

            if (newResult==DUE) {
                // 1. error is detected by T1EC
                int nonZeroChecksumCount = 0;
                Fault *nonZeroChecksumFault = NULL;

                Fault *newFault = fd->operationalFaultList.back();
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
                            nonZeroChecksumFault = *it;
                        }
                    } else {
                        nonZeroChecksumCount++;
                        nonZeroChecksumFault = *it;
                    }
                }
                if (nonZeroChecksumCount==1) {
                    // correct it
                    decoded.setSymbol(16, nonZeroChecksumFault->getChipID(), 0);

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

    assert(result!=NE);

    return result;
}

