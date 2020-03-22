/**
 * @file: linear_codec.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * CODEC declaration (linear codec)
 */

#ifndef __LINEAR_CODEC_HH__
#define __LINEAR_CODEC_HH__

#include <stdint.h>
#include "message.hh"
#include "codec.hh"
#include "gf.hh"

template <int p, int m>
class LinearCodec : public Codec {
    // Constructor / destructor
public:
    LinearCodec(const char *name, int symN, int symR);
    ~LinearCodec();
    // member methods
public:
    virtual void encode(Block *data, ECCWord *encoded);
    virtual ErrorType decode(ECCWord *msg, ECCWord *decoded) = 0;
protected:
    bool genSyndrome(ECCWord *msg);
    void print(FILE *fd);
    // member fields
protected:
    int symN, symK, symR;
    GFElem<p, m>* syndrom;
    // P matrix: rxk
    GFElem<p, m>* gMatrix;       // G: k x n matrix (1D representation) / Identity matrix at MSB
    GFElem<p, m>* hMatrix;       // H: r x n matrix (1D representation) / Identity matrix at LSB
};

#endif /* __LINEAR_CODEC_HH__ */
