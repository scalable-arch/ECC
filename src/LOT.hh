/**
 * @file: LOT.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * LOT-ECC declaration
 */

#ifndef __LOT_HH__
#define __LOT_HH__

#include "ECC.hh"

//------------------------------------------------------------------------------
// LOT-ECC: LOcalized and Tiered Reliability Mechanisms
//------------------------------------------------------------------------------
class LOTECC : public ECC {
public:
    LOTECC();

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
};

#endif /* __LOT_HH__ */
