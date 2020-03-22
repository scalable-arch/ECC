/**
 * @file: MultiECC.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * LOT-ECC declaration
 */

#ifndef __MULTI_ECC_HH__
#define __MULTI_ECC_HH__

#include "ECC.hh"

//------------------------------------------------------------------------------
// “Low-power, low-storage-overhead chipkill correct via multi-line error correction"
//------------------------------------------------------------------------------
class MultiECC : public ECC {
public:
    MultiECC();

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
};

#endif /* __MULTI_ECC_HH__ */
