/**
 * @file: gf.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * GF declaration 
 */

#ifndef __GF_HH__
#define __GF_HH__

#include <assert.h>
#include "util.hh"

// F16 = F(2^4)
// primitive polynomial: x^4 + x + 1 = 0
// elements
// vector form {0, 1, 2, 3, 4, 5, ... 15}
// polynomial form: {0, 1, a, a+1, a^2, a^2+1, a^2+a, a^2+a+1, ... , a^3+a^2+a+1}
// power form: {0, 1, a, a^2, a^3, a^4, a^5, a^6, a^7, ..., a^14}

// F64 = F(2^8)
// primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1
// vector form {0, 1, 2, 3, 4, 5, ... 255}
// polynomial form: {0, 1, a, a+1, a^2, a^2+1, a^2+a, a^2+a+1, ... , a^7+a^6+a^5+a^4+a^3+a^2+a+1}
// power form: {0, 1, a, a^2, a^3, a^4, a^5, a^6, a^7, ..., a^62}

//------------------------------------------------------------------------------
template <int p, int m>
class GF {
    // constructor / destructor
public:
    GF();

    // member functions
public:
    POLY pickPrimitivePoly();
    POLY genMinimalPoly(int alphaPower);

    inline bool isZeroPoly(POLY poly) const { return (poly==0); }
    inline bool isZeroIndex(INDEX index) const { return (index==maxIndex); }
    INDEX getZeroIndex() const { return maxIndex; }
    POLY index2poly(INDEX index) const { return index2polyTable[index]; }
    INDEX poly2index(POLY poly) const { return poly2indexTable[poly]; }

    void print() const;

    // member fields
public:
    INDEX maxIndex;
    POLY primitivePoly;

    POLY index2polyTable[1<<m];
    INDEX poly2indexTable[1<<m];
};

//------------------------------------------------------------------------------
template <int p, int m>
class GFElem {
    // constructor / destructor
public:
    GFElem(): indexValue(gf.getZeroIndex()) {}
    GFElem(int _indexValue): indexValue(_indexValue) {}
    // member functions
public:
    GFElem& operator=(const GFElem<p, m>& rhs);
    GFElem& operator+=(const GFElem<p, m>& rhs);
    GFElem& operator-=(const GFElem<p, m>& rhs);
    GFElem& operator*=(const GFElem<p, m>& rhs);
    GFElem& operator/=(const GFElem<p, m>& rhs);
    GFElem& operator%=(const GFElem<p, m>& rhs);
    GFElem& operator^=(const int rhs);
    bool operator==(const GFElem<p, m>& rhs) const;
    bool operator!=(const GFElem<p, m>& rhs) const;
    GFElem operator+(const GFElem<p, m>& rhs);
    GFElem operator-(const GFElem<p, m>& rhs);
    GFElem operator*(const GFElem<p, m>& rhs);
    GFElem operator/(const GFElem<p, m>& rhs);
    GFElem operator^(const int rhs);

    bool isZero() const { return indexValue == gf.getZeroIndex(); }
    void setIndexValue(INDEX i) { indexValue = i; }
    INDEX getIndexValue() { return indexValue; }
    void setPolyValue(POLY _p) { indexValue = gf.poly2index(_p); }
    POLY getPolyValue() { return gf.index2poly(indexValue); }
    void setValue(int v) { if (v==0) indexValue = gf.getZeroIndex(); else indexValue = v-1; }
    int getValue() { if (indexValue==gf.getZeroIndex()) return 0; else return indexValue+1; }
    // member fields
public:
    static GF<p, m> gf;
    static GFElem ZERO;
private:
    int indexValue;
};

//------------------------------------------------------------------------------
class Block;

template <int p, int m>
class GFPoly {
    // constructor / destructor
public:
    GFPoly(int degree);
    GFPoly(Block* msg);
    ~GFPoly();
    // member methods
public:
    GFPoly& operator=(const GFPoly<p, m>& rhs);
    GFPoly& operator+=(const GFPoly<p, m>& rhs);
    GFPoly& operator-=(const GFPoly<p, m>& rhs);
    GFPoly& operator*=(const GFPoly<p, m>& rhs);
    GFPoly& operator*=(const GFElem<p, m>& rhs);
    GFPoly& operator/=(const GFPoly<p, m>& rhs);
    GFPoly& operator/=(const GFElem<p, m>& rhs);
    GFPoly& operator%=(const GFPoly<p, m>& rhs);
    GFPoly& operator^=(const int rhs);
    GFPoly& operator<<=(const int rhs);
    GFElem<p, m>& operator[](const int position) { return coeffArr[position]; }
    bool operator==(const GFPoly<p, m>& rhs) const;
    bool operator!=(const GFPoly<p, m>& rhs) const;
    GFPoly operator+(const GFPoly<p, m>& rhs);
    GFPoly operator-(const GFPoly<p, m>& rhs);
    GFPoly operator*(const GFPoly<p, m>& rhs);
    GFPoly operator*(const GFElem<p, m>& rhs);
    GFPoly operator/(const GFPoly<p, m>& rhs);
    GFPoly operator/(const GFElem<p, m>& rhs);
    GFPoly operator%(const GFPoly<p, m>& rhs);
    GFPoly operator<<(const int rhs);

    int getDegree() { return degree; }
    void setCoeff(int position, GFElem<p, m> e) { coeffArr[position] = e; }
    GFElem<p, m> getCoeff(int position) { assert(position>=0); return coeffArr[position]; }

    void print() const;
    // member fields
public:
    int degree;
    GFElem<p, m>* coeffArr;
};

#endif /* __GF_HH__ */
