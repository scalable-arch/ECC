/**
 * @file: rs.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * CODEC declaration (RS)
 */

#ifndef __RS_HH__
#define __RS_HH__

#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <list>
#include <algorithm>
#include "message.hh"
#include "codec.hh"
#include "gf.hh"
#include "gf.cc"            // template
#include "linear_codec.hh"
#include "linear_codec.cc"  // template

//#define SYNDROME_TABLE

// maximum code length : n (= 2^m -1)
// check-symbol length : r
template <int p, int m>
class RS : public Codec {        // GF2
    // Constructor / destructor
public:
    RS(const char *name, int _symN, int _symR, int _symT, int _symB=4)
        : Codec(name, m*_symN, m*_symR), gPoly(0) {
        symN = _symN;
        symK = _symN - _symR;
        symR = _symR;
        symT = _symT;
		symB = _symB; // GONG: number of symbols for burst decoding (correction)

        syndrome = new GFElem<p, m>[symR];

        // 1. length check
        // Maximum code length in bits: n_max = 2^m -1;
        // Minimum code length in bits: n_min = 2^(m-1);
        int n_max = (1<<m) - 1;
        int n_min = (1<<(m-1));

        if (symN > n_max) {
            printf("Invalid n for GF: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
            //assert(0);
        }
        //if (symN < n_min) {
        //    printf("Shortened code: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
        //}

        // 2. generate generator polynomial
        genGenPoly();

        //findHD3NeighborCodewords();

#ifdef SYNDROME_TABLE
        findCorrectableSyndrome();
#endif /* SYNDROME_TABLE */
    }
    ~RS() {
        delete[] syndrome;
    }

    // member methods
public:
    void encode(Block *data, ECCWord *encoded) {
        GFPoly<p, m> dataPoly(data);
        GFPoly<p, m> remainderPoly(symR);

        dataPoly <<= symR;     // shift by (n-k)

        remainderPoly = dataPoly % gPoly;

        for (int i=0; i<symK; i++) {
            encoded->setSymbol(m, i+symR, data->getSymbol(m, i));
        }
        for (int i=0; i<symR; i++) {
            encoded->setSymbol(m, i, remainderPoly[i].getValue());
        }
    }
    ErrorType decode(ECCWord *msg, ECCWord *decoded, std::set<int>* correctedPos) {
        // step 1: copy the message data
        decoded->clone(msg);

        // step 2: generate syndrome
        bool synError = genSyndrome(msg);

        // Step 3: if all of syndrome bits are zero, the word can be assumed to be error free
        if (synError) {
#ifdef SYNDROME_TABLE
            assert(m*symR <= 64);
            uint64_t syndromeKey = 0ull;
            for (int i=0; i<symR; i++) {
                syndromeKey ^= syndrome[i].getValue() << (i*m);
            }
            auto got = correctableSyndromes.find(syndromeKey);
            if (got != correctableSyndromes.end()) {
                uint64_t correctionInfo = got->second;
                while (correctionInfo != 0ull) {
                    decoded->invSymbol(m, (correctionInfo>>8)&0xFF, correctionInfo&0xFF);    // position, value
                    correctionInfo >>= 16;
                }
                insertCorrectionInfo((correctionInfo>>8)&0xFF, correctionInfo&0xFF);
                if (decoded->isZero()) {
                    return CE;
                } else {
                    return SDC;
                }
            }

            return DUE;
#else
            GFPoly<p, m> elp (symR);
            GFPoly<p, m> prev_elp (symR);
            GFPoly<p, m> reg (symR);
            int ll = 0;
            int mm = 1;
            GFElem<p, m> prev_discrepancy;
            int count = 0;
            int root[symT];
            int loc[symT];

            elp[0].setPolyValue(1);
            prev_elp[0].setPolyValue(1);
            prev_discrepancy.setPolyValue(1);

            // Berlekamp–Massey algorithm from Wikipedia
            for (int n=0; n<symR; n++) {
                GFElem<p, m> discrepancy = syndrome[n];
                for (int i=1; i<=ll; i++) {
                    discrepancy += elp[i]*syndrome[n-i];
                }
                if (discrepancy.isZero()) {
                    mm++;
                } else if ((2*ll) <= n) {
                    GFPoly<p, m> temp(symR);
                    temp = elp;
                    elp -= (prev_elp << mm) * discrepancy / prev_discrepancy;
                    ll = n + 1 - ll;
                    prev_elp = temp;
                    prev_discrepancy = discrepancy;
                    mm = 1;
                } else {
                    elp -= (prev_elp << mm) * discrepancy / prev_discrepancy;
                    mm++;
                }
            }
            //printf("L=%d ", ll);
            //elp.print();

            if (ll <= symT) { // can correct error
                // Chien search
                reg = elp;
                for (int i=0; i<(1<<m)-1; i++) {
                    GFElem<p, m> q;
                    q.setPolyValue(0);
                    for (int j=0; j<=ll; j++) {
                        q += reg[j];
                        reg[j] *= GFElem<p, m>(j);
                    }

                    if (q.isZero()) {
                        root[count] = i;
                        loc[count] = (i!=0) ? ((1<<m)-1)-i : 0;
                        if (loc[count] >= symN) {
                            return DUE;
                        }
                        count++ ;
                    }
                }
            } else {
                return DUE;
            }

            if (count==ll) {
                // Forney algorithm
                GFElem<p, m> z[symT+1];
                for (int i=1; i<=ll; i++) {
                    z[i] = syndrome[i-1] + elp[i];
                    for (int j=1; j<i; j++) {
                        z[i] += syndrome[j-1]* elp[i-j];
                    }
                }

                for (int i=0; i<ll; i++) {
                    GFElem<p, m> err;
                    err.setIndexValue(0);
                    for (int j=1; j<=ll; j++) {
                        err += z[j]*GFElem<p, m>(j*root[i]);
                    }
                    if (!err.isZero()) {
                        GFElem<p, m> q;
                        q.setIndexValue(0);
                        for (int j=0; j<ll; j++) {
                            if (i!=j) {
                                GFElem<p, m> temp((loc[j]+root[i]) % ((1<<m)-1));
                                GFElem<p, m> temp1;
                                temp1.setIndexValue(0);
                                temp += temp1;
                                q *= temp;
                            }
                        }
                        err /= q;
                    }
                    //printf("STG#2: i:%d LOC:%d ERR:%d(%d) (ll%d)\n", i, loc[i], err.getIndexValue(), err.getPolyValue(), ll);
                    insertCorrectionInfo(loc[i], err.getIndexValue()+1);
                    decoded->invSymbol(m, loc[i], err.getIndexValue()+1);       // position, value
                    if (correctedPos!=NULL) {
                        correctedPos->insert(loc[i]);
                    }
                }
                if (decoded->isZero()) {
                    return CE;
                } else {
                    return SDC;
                }
            } else {
                return DUE;
            }
#endif /* SYNDROME_TABLE */
        } else {
            return SDC;
        }
    }
    ErrorType decodeBurstDUO64bx4(ECCWord *msg, ECCWord *decoded, int burstLength, std::set<int>* correctedPos) {

        // step 1: copy the message data
        decoded->clone(msg);

        // step 2: generate syndrome
        bool synError = genSyndrome(msg);
        
				// Step 3: if all of syndrome bits are zero, the word can be assumed to be error free
        if (synError) {
            GFElem<p, m> s[symR];
            GFElem<p, m> e[symB];

            // assumption: 4-burst symbol errors aligned to 4 symbol boundaries
            for (int startPos=0; startPos < 64; startPos+=4) {
                // error location: startPos+0, +1, +2, +3
                // syndromes are shifted by startPos*N
                s[0] = syndrome[0] / GFElem<p,m>((startPos)*1);
                s[1] = syndrome[1] / GFElem<p,m>((startPos)*2);
                s[2] = syndrome[2] / GFElem<p,m>((startPos)*3);
                s[3] = syndrome[3] / GFElem<p,m>((startPos)*4);
								
                // error values
								// GONG: 
								// coefficients can be calculated by gaussian elimination (generating inverse matrix) on Galois Field.
								// A * e = s  -> e = A^(-1) * s
                e[0] = s[0]*GFElem<p,m>(218)+s[1]*GFElem<p,m>(505)+s[2]*GFElem<p,m>(503)+s[3]*GFElem<p,m>(212);
                e[1] = s[0]*GFElem<p,m>(504)+s[1]*GFElem<p,m>(225)+s[2]*GFElem<p,m>(201)+s[3]*GFElem<p,m>(499);
                e[2] = s[0]*GFElem<p,m>(501)+s[1]*GFElem<p,m>(200)+s[2]*GFElem<p,m>(221)+s[3]*GFElem<p,m>(497);
                e[3] = s[0]*GFElem<p,m>(209)+s[1]*GFElem<p,m>(497)+s[2]*GFElem<p,m>(496)+s[3]*GFElem<p,m>(206);

								// recalculate syndrom
                GFElem<p, m> s2[8];
                s2[4] = (e[0] + e[1]*GFElem<p,m>(5) + e[2]*GFElem<p,m>(10)+ e[3]*GFElem<p,m>(15))*GFElem<p,m>(startPos*5);
                s2[5] = (e[0] + e[1]*GFElem<p,m>(6) + e[2]*GFElem<p,m>(12)+ e[3]*GFElem<p,m>(18))*GFElem<p,m>(startPos*6);
                s2[6] = (e[0] + e[1]*GFElem<p,m>(7) + e[2]*GFElem<p,m>(14)+ e[3]*GFElem<p,m>(21))*GFElem<p,m>(startPos*7);
                s2[7] = (e[0] + e[1]*GFElem<p,m>(8) + e[2]*GFElem<p,m>(16)+ e[3]*GFElem<p,m>(24))*GFElem<p,m>(startPos*8);
								
								//
                //printf("-- %2d : %03x %03x %03x %03x : (%03x %03x) (%03x %03x) (%03x %03x)\n",
                //                startPos,
                //                e[3].getIndexValue()+1,
                //                e[2].getIndexValue()+1,
                //                e[1].getIndexValue()+1,
                //                e[0].getIndexValue()+1,
                //                syndrome[4].getIndexValue(),
                //                s2[4].getIndexValue(),
                //                syndrome[5].getIndexValue(),
                //                s2[5].getIndexValue(),
                //                syndrome[6].getIndexValue(),
                //                s2[6].getIndexValue());
                //
								
								if ((syndrome[4]==s2[4])&&(syndrome[5]==s2[5])&&(syndrome[6]==s2[6])) {
                    ECCWord temp = {64*9, 57*9};
                    temp.clone(msg);
                    temp.invSymbol(m, startPos,   e[0].getIndexValue()+1);
                    temp.invSymbol(m, startPos+1, e[1].getIndexValue()+1);
                    temp.invSymbol(m, startPos+2, e[2].getIndexValue()+1);
                    temp.invSymbol(m, startPos+3, e[3].getIndexValue()+1);

                    bool parity = 0, parityError;
                    for (int i=0; i<15; i++) {
                        parity ^= temp.getBit(36*i);
                    }
                    parityError = (parity!=temp.getBit(512));

                    if (parityError) {
                        continue;
                    }

                    insertCorrectionInfo(startPos,   e[0].getIndexValue()+1);
                    insertCorrectionInfo(startPos+1, e[1].getIndexValue()+1);
                    insertCorrectionInfo(startPos+2, e[2].getIndexValue()+1);
                    insertCorrectionInfo(startPos+3, e[3].getIndexValue()+1);
                    decoded->invSymbol(m, startPos,   e[0].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+1, e[1].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+2, e[2].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+3, e[3].getIndexValue()+1);
//GONG
//decoded->print();
                    if (decoded->isZero()) {
                        return CE;
                    } else {
                        return SDC;
                    }
                }
            }
            return DUE;
        } else {
            return SDC;
        }
    }//end of decodeBurst
    
	ErrorType decodeBurstDUO36bx4(ECCWord *msg, ECCWord *decoded, int burstLength, std::set<int>* correctedPos) {

        // step 1: copy the message data
        decoded->clone(msg);

        // step 2: generate syndrome
        bool synError = genSyndrome(msg);
        
				// Step 3: if all of syndrome bits are zero, the word can be assumed to be error free
        if (synError) {
            GFElem<p, m> s[symR];
            GFElem<p, m> e[symB];

			bool odd=true; //odd-even symbol?
            for (int startPos=0; startPos < symN; startPos+=symB) {

//								printf("startPos: %d\n", startPos);
                // error location: startPos+0, +1, +2, +3
                // syndromes are shifted by startPos*N
                s[0] = syndrome[0] / GFElem<p,m>(((startPos)*1)%255);
                s[1] = syndrome[1] / GFElem<p,m>(((startPos)*2)%255);
                s[2] = syndrome[2] / GFElem<p,m>(((startPos)*3)%255);
                s[3] = syndrome[3] / GFElem<p,m>(((startPos)*4)%255);
                s[4] = syndrome[4] / GFElem<p,m>(((startPos)*5)%255);
                s[5] = syndrome[5] / GFElem<p,m>(((startPos)*6)%255);
                s[6] = syndrome[6] / GFElem<p,m>(((startPos)*7)%255);
                s[7] = syndrome[7] / GFElem<p,m>(((startPos)*8)%255);
                s[8] = syndrome[8] / GFElem<p,m>(((startPos)*9)%255);
						
								//coefficients		
//						GFElem<p,m> c[symB*symB] =	{17	,184	,239	,201	,234	,192	,221	,157	,236,	
//													 183	,102	,184	,4		,183	,205	,117	,96	,148	,	
//													 237	,183	,226	,79	,104	,46	,150	,108	,203		,	
//													 198	,2		,78		,148	,110	,152	,37	,187	,165	,	
//													 230	,180	,102	,109	,235	,101	,86	,156	,198	,	
//													 187	,201	,43		,150	,100	,130	,52	,223	,156	,	
//													 215	,112	,146	,34	,84	,51	,190	,139	,185			,	
//													 150	,90		,103	,183	,153	,221	,138	,48	,121	,	
//													 228	,141	,197	,160	,194	,153	,183	,120	,200};
                // error values
								// GONG: 
								// coefficients can be calculated by gaussian elimination (generating inverse matrix) on Galois Field.
								// A * e = s  -> e = A^(-1) * s
                e[0] = s[0]*GFElem<p,m>( 17)+s[1]*GFElem<p,m>(184)+s[2]*GFElem<p,m>(239)+s[3]*GFElem<p,m>(201)+s[4]*GFElem<p,m>(234)+s[5]*GFElem<p,m>(192)+s[6]*GFElem<p,m>(221)+s[7]*GFElem<p,m>(157)+s[8]*GFElem<p,m>(236);
                e[1] = s[0]*GFElem<p,m>(183)+s[1]*GFElem<p,m>(102)+s[2]*GFElem<p,m>(184)+s[3]*GFElem<p,m>(  4)+s[4]*GFElem<p,m>(183)+s[5]*GFElem<p,m>(205)+s[6]*GFElem<p,m>(117)+s[7]*GFElem<p,m>( 96)+s[8]*GFElem<p,m>(148);
                e[2] = s[0]*GFElem<p,m>(237)+s[1]*GFElem<p,m>(183)+s[2]*GFElem<p,m>(226)+s[3]*GFElem<p,m>( 79)+s[4]*GFElem<p,m>(104)+s[5]*GFElem<p,m>( 46)+s[6]*GFElem<p,m>(150)+s[7]*GFElem<p,m>(108)+s[8]*GFElem<p,m>(203);
                e[3] = s[0]*GFElem<p,m>(198)+s[1]*GFElem<p,m>(  2)+s[2]*GFElem<p,m>( 78)+s[3]*GFElem<p,m>(148)+s[4]*GFElem<p,m>(110)+s[5]*GFElem<p,m>(152)+s[6]*GFElem<p,m>( 37)+s[7]*GFElem<p,m>(187)+s[8]*GFElem<p,m>(165);
                e[4] = s[0]*GFElem<p,m>(230)+s[1]*GFElem<p,m>(180)+s[2]*GFElem<p,m>(102)+s[3]*GFElem<p,m>(109)+s[4]*GFElem<p,m>(235)+s[5]*GFElem<p,m>(101)+s[6]*GFElem<p,m>( 86)+s[7]*GFElem<p,m>(156)+s[8]*GFElem<p,m>(198);
                e[5] = s[0]*GFElem<p,m>(187)+s[1]*GFElem<p,m>(201)+s[2]*GFElem<p,m>( 43)+s[3]*GFElem<p,m>(150)+s[4]*GFElem<p,m>(100)+s[5]*GFElem<p,m>(130)+s[6]*GFElem<p,m>( 52)+s[7]*GFElem<p,m>(223)+s[8]*GFElem<p,m>(156);
                e[6] = s[0]*GFElem<p,m>(215)+s[1]*GFElem<p,m>(112)+s[2]*GFElem<p,m>(146)+s[3]*GFElem<p,m>( 34)+s[4]*GFElem<p,m>( 84)+s[5]*GFElem<p,m>( 51)+s[6]*GFElem<p,m>(190)+s[7]*GFElem<p,m>(139)+s[8]*GFElem<p,m>(185);
                e[7] = s[0]*GFElem<p,m>(150)+s[1]*GFElem<p,m>( 90)+s[2]*GFElem<p,m>(103)+s[3]*GFElem<p,m>(183)+s[4]*GFElem<p,m>(153)+s[5]*GFElem<p,m>(221)+s[6]*GFElem<p,m>(138)+s[7]*GFElem<p,m>( 48)+s[8]*GFElem<p,m>(121);
                e[8] = s[0]*GFElem<p,m>(228)+s[1]*GFElem<p,m>(141)+s[2]*GFElem<p,m>(197)+s[3]*GFElem<p,m>(160)+s[4]*GFElem<p,m>(194)+s[5]*GFElem<p,m>(153)+s[6]*GFElem<p,m>(183)+s[7]*GFElem<p,m>(120)+s[8]*GFElem<p,m>(200);

                // recalculate syndrom
                GFElem<p, m> s2[symR];
                s2[ 9] = (e[0] + e[1]*GFElem<p,m>(10) + e[2]*GFElem<p,m>(20)+ e[3]*GFElem<p,m>(30) + e[4]*GFElem<p,m>(40)+ e[5]*GFElem<p,m>(50) + e[6]*GFElem<p,m>(60)+ e[7]*GFElem<p,m>(70)+ e[8]*GFElem<p,m>(80))*GFElem<p,m>((startPos*10)%255);
                s2[10] = (e[0] + e[1]*GFElem<p,m>(11) + e[2]*GFElem<p,m>(22)+ e[3]*GFElem<p,m>(33) + e[4]*GFElem<p,m>(44)+ e[5]*GFElem<p,m>(55) + e[6]*GFElem<p,m>(66)+ e[7]*GFElem<p,m>(77)+ e[8]*GFElem<p,m>(88))*GFElem<p,m>((startPos*11)%255);
                s2[11] = (e[0] + e[1]*GFElem<p,m>(12) + e[2]*GFElem<p,m>(24)+ e[3]*GFElem<p,m>(36) + e[4]*GFElem<p,m>(48)+ e[5]*GFElem<p,m>(60) + e[6]*GFElem<p,m>(72)+ e[7]*GFElem<p,m>(84)+ e[8]*GFElem<p,m>(96))*GFElem<p,m>((startPos*12)%255);
								
				   //
				   //msg->print();
                   //printf("-- %2d : %02x %02x %02x %02x %02x %02x %02x %02x %02x : (%02x %02x) (%02x %02x) (%02x %02x)\n",
                   //startPos,
                   //e[8].getIndexValue()+1,
                   //e[7].getIndexValue()+1,
                   //e[6].getIndexValue()+1,
                   //e[5].getIndexValue()+1,
                   //e[4].getIndexValue()+1,
                   //e[3].getIndexValue()+1,
                   //e[2].getIndexValue()+1,
                   //e[1].getIndexValue()+1,
                   //e[0].getIndexValue()+1,
                   //syndrome[9].getIndexValue(),
                   //s2[9].getIndexValue(),
                   //syndrome[10].getIndexValue(),
                   //s2[10].getIndexValue(),
                   //syndrome[11].getIndexValue(),
                   //s2[11].getIndexValue());
                
								
								if ((syndrome[9]==s2[9])&&(syndrome[10]==s2[10])&&(syndrome[11]==s2[11])) {
                    ECCWord temp = {76*8, 64*8};
                    temp.clone(msg);
                    temp.invSymbol(m, startPos,   e[0].getIndexValue()+1);
                    temp.invSymbol(m, startPos+1, e[1].getIndexValue()+1);
                    temp.invSymbol(m, startPos+2, e[2].getIndexValue()+1);
                    temp.invSymbol(m, startPos+3, e[3].getIndexValue()+1);
                    temp.invSymbol(m, startPos+4, e[4].getIndexValue()+1);
                    temp.invSymbol(m, startPos+5, e[5].getIndexValue()+1);
                    temp.invSymbol(m, startPos+6, e[6].getIndexValue()+1);
                    temp.invSymbol(m, startPos+7, e[7].getIndexValue()+1);
                    temp.invSymbol(m, startPos+8, e[8].getIndexValue()+1);
										
                    bool parity0, parity1, parity2, parity3, parityError;
										parity0 = parity1 = parity2 = parity3 = 0;
                    for (int i=0; i<9*16; i++) {
                        parity0 ^= temp.getBit(4*i+0);
                        parity1 ^= temp.getBit(4*i+1);
                        parity2 ^= temp.getBit(4*i+2);
                        parity3 ^= temp.getBit(4*i+3);
                    }
                    parityError = (parity0 != temp.getBit(4*9*16    )) 
																| (parity1 != temp.getBit(4*9*16 + 1))
																| (parity2 != temp.getBit(4*9*16 + 2))
																| (parity3 != temp.getBit(4*9*16 + 3));

                    if (parityError) {
                        continue;
                    }

                    insertCorrectionInfo(startPos,   e[0].getIndexValue()+1);
                    insertCorrectionInfo(startPos+1, e[1].getIndexValue()+1);
                    insertCorrectionInfo(startPos+2, e[2].getIndexValue()+1);
                    insertCorrectionInfo(startPos+3, e[3].getIndexValue()+1);
                    insertCorrectionInfo(startPos+4, e[4].getIndexValue()+1);
                    insertCorrectionInfo(startPos+5, e[5].getIndexValue()+1);
                    insertCorrectionInfo(startPos+6, e[6].getIndexValue()+1);
                    insertCorrectionInfo(startPos+7, e[7].getIndexValue()+1);
                    insertCorrectionInfo(startPos+8, e[8].getIndexValue()+1);
                    decoded->invSymbol(m, startPos,   e[0].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+1, e[1].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+2, e[2].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+3, e[3].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+4, e[4].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+5, e[5].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+6, e[6].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+7, e[7].getIndexValue()+1);
                    decoded->invSymbol(m, startPos+8, e[8].getIndexValue()+1);
//GONG
//decoded->print();
                    if (decoded->isZero()) {
                        return CE;
                    } else {
                        return SDC;
                    }
                }
								
				if(odd){
					startPos--;
					odd = false;
				}else odd = true;
            }
            return DUE;
        } else {
            return SDC;
        }
    }//end of decodeBurst

	//GONG: Ideal erasure decoding + RS correction trials after erasure correction
    ErrorType EraseDUO36bx4(ECCWord *msg, ECCWord *decoded, int burstLength, std::set<int>* correctedPos, Codec* postBE){
		ErrorType tmp_result = DUE;
		bool hasSDC = false;
		//msg->print();
        decoded->clone(msg);
		//counting clean symbols
		int zero_count = 0;
        for(int startPos=0; startPos < symN; startPos++) {
			if(msg->getSymbol(m, startPos) == 0) zero_count++;
		}
		//Checking whether erasure decoding is available (oracle) 
		//printf("zero_count: %d %d\n", zero_count, symN-symR);		
		if(zero_count >= symN - symR){
			bool odd=true; 
        	for(int startPos=0; startPos < symN; startPos+=symB) {
            	ECCWord tmp_msg = {76*8, 64*8};
            	ECCWord tmp_decoded = {76*8, 64*8};
				tmp_msg.clone(msg);
				//assume that erasure decoding is available
				for(int j=startPos; j<startPos+symB; j++){
					tmp_msg.setSymbol(m, j, 0);		
				}
				//printf("startPos: %d\n", startPos);
				std::set<int> tmp_cPos;
				tmp_result = postBE->decode(&tmp_msg, &tmp_decoded, &tmp_cPos);
				//decoded->clone(&tmp_decoded);
				if(tmp_result!=DUE){
					//parity check
                    bool parity0, parity1, parity2, parity3, parityError;
					parity0 = parity1 = parity2 = parity3 = 0;
                    for (int i=0; i<9*16; i++) {
                        parity0 ^= tmp_decoded.getBit(4*i+0);
                        parity1 ^= tmp_decoded.getBit(4*i+1);
                        parity2 ^= tmp_decoded.getBit(4*i+2);
                        parity3 ^= tmp_decoded.getBit(4*i+3);
                    }
                    parityError = (parity0 != tmp_decoded.getBit(4*9*16    )) 
								| (parity1 != tmp_decoded.getBit(4*9*16 + 1))
								| (parity2 != tmp_decoded.getBit(4*9*16 + 2))
								| (parity3 != tmp_decoded.getBit(4*9*16 + 3));
					if(!parityError){	
						decoded->clone(&tmp_decoded);
						
						//Due to zero syndrome after erasure correction, postBE can return SDC.
						//Here we double-check whether decoding corrects all errors by usign isZero().
						if(decoded->isZero()){
							assert(tmp_cPos.size()<=1);
							for(std::set<int>::iterator it = tmp_cPos.begin(); it!=tmp_cPos.end(); ++it){
								correctedPos->insert(*it);
							}
							return CE;
						} else{
							//msg->print();
							//tmp_msg.print();	
							//decoded->print();
							hasSDC = true;
						}
					}else{
						//return DUE;		
					}
				}		
				
				if(odd){
					startPos--;
					odd = false;
				}else odd = true;
			}
		}
		//Reaching here implies the ECC Word cannot be corrected or SDC.
		if(hasSDC) return SDC;
		else return DUE;					
	}//End of BurstErase()

    virtual bool genSyndrome(ECCWord *msg) {
        bool synError = false;

        GFElem<p, m> msgElemArr[symN];
        for (int i=0; i<symN; i++) {
            msgElemArr[i].setValue(msg->getSymbol(m, i));
        }

        for (int i=0; i<symR; i++) {
            syndrome[i].setValue(0);
            for (int j=0; j<symN; j++) {
                syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+1)*j)%((1<<m)-1));
            }
            if (!syndrome[i].isZero()) {
                synError = true;
            }
        }
        return synError;
    }

private:
    void genGenPoly() {
        gPoly.setCoeff(0, 0);   // 1

        GFPoly<p, m> temp(1);   // x + a^i
        temp.setCoeff(1, 0);
        for (int i=0; i<symR; i++) {
            temp.setCoeff(0, i+1);
            gPoly *= temp;
        }

        //printf("Generator poly: %d %d\n", m, symR);
        //gPoly.print();
    }

    void findHD3NeighborCodewords() {
        ECCWord msg = {Codec::getBitN(), Codec::getBitK()};
        ECCWord decoded = {Codec::getBitN(), Codec::getBitK()};

        int position1, value1, position2, value2, position3, value3;
        uint64_t neighbor_cnt = 0;

        for (int position1=0; position1<symN; position1++) {
            for (int value1=1; value1<(1<<m); value1++) {
                for (int position2=position1+1; position2<symN; position2++) {
                    for (int value2=1; value2<(1<<m); value2++) {
                        for (int position3=position2+1; position3<symN; position3++) {
                            for (int value3=1; value3<(1<<m); value3++) {
                                msg.reset();
                                msg.invSymbol(m, position1, value1);
                                msg.invSymbol(m, position2, value2);
                                msg.invSymbol(m, position3, value3);

                                ErrorType result = decode(&msg, &decoded);

                                if (result==SDC) {
                                    neighbor_cnt++;
                                }
                            }
                        }
                    }
                }
            }
        }

        printf("Neighbors (HD=3): %lld\n", neighbor_cnt);
    }
#ifdef SYNDROME_TABLE
    bool findCorrectableSyndrome() {
        // find out correctable syndroms
        ECCWord msg = {Codec::getBitN(), Codec::getBitK()};
        ECCWord decoded = {Codec::getBitN(), Codec::getBitK()};
        for (int position1=0; position1<symN; position1++) {
            for (int value1=1; value1<(1<<m); value1++) {
                msg.reset();
                msg.invSymbol(m, position1, value1);

                ErrorInfo errorInfo = (((uint64_t) position1)<<8)
                                    | ((uint64_t) value1);
                //ErrorInfo errorInfo = 0ull;
                //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                if (result==SDC) return false;

                if (symT>=2) {
                    for (int position2=position1+1; position2<symN; position2++) {
                        for (int value2=1; value2<(1<<m); value2++) {
                            msg.reset();
                            msg.invSymbol(m, position1, value1);
                            msg.invSymbol(m, position2, value2);

                            ErrorInfo errorInfo = (((uint64_t) position2)<<24)
                                                | (((uint64_t) value2)<<16)
                                                | (((uint64_t) position1)<<8)
                                                | ((uint64_t) value1);
                            //ErrorInfo errorInfo;
                            //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                            //errorInfo.insert(std::make_pair<int, int>((int) position2, (int) value2));
                            ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                            if (result==SDC) return false;

                            if (symT>=3) {
                                printf("--(%d, %d)\n", position1, position2);

                                for (int position3=position2+1; position3<symN; position3++) {
                                    for (int value3=1; value3<(1<<m); value3++) {
                                        msg.reset();
                                        msg.invSymbol(m, position1, value1);
                                        msg.invSymbol(m, position2, value2);
                                        msg.invSymbol(m, position3, value3);

                                        ErrorInfo errorInfo = (((uint64_t) position3)<<40)
                                                            | (((uint64_t) value3)<<32)
                                                            | (((uint64_t) position2)<<24)
                                                            | (((uint64_t) value2)<<16)
                                                            | (((uint64_t) position1)<<8)
                                                            | ((uint64_t) value1);
                                        //ErrorInfo errorInfo;
                                        //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                                        //errorInfo.insert(std::make_pair<int, int>((int) position2, (int) value2));
                                        //errorInfo.insert(std::make_pair<int, int>((int) position3, (int) value3));
                                        ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                                        if (result==SDC) return false;


                                        if (symT>=4) {
                                            for (int position4=position3+1; position4<symN; position4++) {
                                                for (int value4=1; value4<(1<<m); value4++) {
                                                    msg.reset();
                                                    msg.invSymbol(m, position1, value1);
                                                    msg.invSymbol(m, position2, value2);
                                                    msg.invSymbol(m, position3, value3);
                                                    msg.invSymbol(m, position4, value4);

                                                    ErrorInfo errorInfo = (((uint64_t) position4)<<56)
                                                                        | (((uint64_t) value4)<<48)
                                                                        | (((uint64_t) position3)<<40)
                                                                        | (((uint64_t) value3)<<32)
                                                                        | (((uint64_t) position2)<<24)
                                                                        | (((uint64_t) value2)<<16)
                                                                        | (((uint64_t) position1)<<8)
                                                                        | ((uint64_t) value1);
                                                    //ErrorInfo errorInfo;
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position2, (int) value2));
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position3, (int) value3));
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position4, (int) value4));
                                                    ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                                                    if (result==SDC) return false;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return true;
    }
    ErrorType decodeCorrectable(ECCWord *msg, ECCWord *decoded, ErrorInfo errorInfo) {
        // step 1: copy the message data
        //decoded->clone(msg);

        // step 2: generate syndrome
        bool synError = genSyndrome(msg);

        // Step 3: if all of syndrome bits are zero, the word can be assumed to be error free
        if (synError) {
            assert(m*symR <= 64);
            uint64_t syndromeKey = 0ull;
            for (int i=0; i<symR; i++) {
                syndromeKey ^= (syndrome[i].getValue() << (i*m));
            }
            correctableSyndromes.insert(std::make_pair<uint64_t, ErrorInfo>((uint64_t) syndromeKey, (ErrorInfo) errorInfo));
            return CE;
        } else {
            return SDC;
        }
    }
#endif /* SYNDROME_TABLE */

    // member fields
public:
    int symN, symK, symR, symT, symB;
    GFPoly<p, m> gPoly;
    GFElem<p, m>* syndrome;
#ifdef SYNDROME_TABLE
    std::unordered_map<uint64_t, ErrorInfo> correctableSyndromes;
#endif /* SYNDROME_TABLE */
};

template <int p, int m>
class RS2 : public Codec {        // GF2
    // Constructor / destructor
public:
    RS2(const char *name, int _symN, int _symR, int _symT, int _pos1, int _pos2, int _pos3)
        : Codec(name, m*_symN, m*_symR), gPoly(0), pos1(_pos1), pos2(_pos2), pos3(_pos3) {
        symN = _symN;
        symK = _symN - _symR;
        symR = _symR;
        symT = _symT;

        syndrome = new GFElem<p, m>[symR];

        // 1. length check
        // Maximum code length in bits: n_max = 2^m -1;
        // Minimum code length in bits: n_min = 2^(m-1);
        int n_max = (1<<m) - 1;
        int n_min = (1<<(m-1));

        if (symN > n_max) {
            printf("Invalid n for GF: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
            //assert(0);
        }
        //if (symN < n_min) {
        //    printf("Shortened code: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
        //}

        // 2. generate generator polynomial
        genGenPoly();

        //findHD3NeighborCodewords();

        //findCorrectableSyndrome();
    }
    ~RS2() {
        delete[] syndrome;
    }

    // member methods
public:
    void encode(Block *data, ECCWord *encoded) {}
    ErrorType decode(ECCWord *msg, ECCWord *decoded, std::set<int>* correctedPos) { return SDC; }
    virtual bool genSyndrome(ECCWord *msg) {
        bool synError = false;

        GFElem<p, m> msgElemArr[symN];
        for (int i=0; i<symN; i++) {
            msgElemArr[i].setPolyValue(msg->getSymbol(m, i));
            //if (msg->getSymbol(m, i)!=0) {
            //    printf("%d %d %d\n", msg->getSymbol(m, i), msgElemArr[i].getValue(), msgElemArr[i].getIndexValue());
            //}
        }

        for (int i=0; i<symR; i++) {
            syndrome[i].setValue(0);
            for (int j=0; j<symN; j++) {
                if (i==0) {
                    syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+pos1)*j)%((1<<m)-1));
                } else if (i==1) {
                    syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+pos2)*j)%((1<<m)-1));
                } else if (i==2) {
                    syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+pos3)*j)%((1<<m)-1));
                }
                //syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+1)*j)%((1<<m)-1));
                //syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+m)*j)%((1<<m)-1));
            }
            if (!syndrome[i].isZero()) {
                synError = true;
            }
        }
        return synError;
    }

    bool findCorrectableSyndrome() {
        // find out correctable syndroms
        ECCWord msg = {Codec::getBitN(), Codec::getBitK()};
        ECCWord decoded = {Codec::getBitN(), Codec::getBitK()};
        for (int position1=0; position1<symN; position1++) {
            for (int value1=1; value1<(1<<m); value1*=2) {
                msg.reset();
                msg.invSymbol(m, position1, value1);

                ErrorInfo errorInfo = (((uint64_t) position1)<<(m))
                                    | ((uint64_t) value1);
                //ErrorInfo errorInfo = 0ull;
                //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                if (result==SDC) return false;

                if (symT>=2) {
                    for (int position2=position1+1; position2<symN; position2++) {
                        for (int value2=1; value2<(1<<m); value2*=2) {
                            msg.reset();
                            msg.invSymbol(m, position1, value1);
                            msg.invSymbol(m, position2, value2);

                            ErrorInfo errorInfo = (((uint64_t) position2)<<(3*m))
                                                | (((uint64_t) value2)<<(2*m))
                                                | (((uint64_t) position1)<<(m))
                                                | ((uint64_t) value1);
                            //ErrorInfo errorInfo;
                            //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                            //errorInfo.insert(std::make_pair<int, int>((int) position2, (int) value2));
                            ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                            if (result==SDC) return false;

                            if (symT>=3) {
                                printf("--(%d, %d)\n", position1, position2);

                                for (int position3=position2+1; position3<symN; position3++) {
                                    for (int value3=1; value3<(1<<m); value3*=2) {
                                        msg.reset();
                                        msg.invSymbol(m, position1, value1);
                                        msg.invSymbol(m, position2, value2);
                                        msg.invSymbol(m, position3, value3);

                                        ErrorInfo errorInfo = (((uint64_t) position3)<<(5*m))
                                                            | (((uint64_t) value3)<<(4*m))
                                                            | (((uint64_t) position2)<<(3*m))
                                                            | (((uint64_t) value2)<<(2*m))
                                                            | (((uint64_t) position1)<<(m))
                                                            | ((uint64_t) value1);
                                        //ErrorInfo errorInfo;
                                        //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                                        //errorInfo.insert(std::make_pair<int, int>((int) position2, (int) value2));
                                        //errorInfo.insert(std::make_pair<int, int>((int) position3, (int) value3));
                                        ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                                        if (result==SDC) return false;

                                        if (symT>=4) {
                                            for (int position4=position3+1; position4<symN; position4++) {
                                                for (int value4=1; value4<(1<<m); value4*=2) {
                                                    msg.reset();
                                                    msg.invSymbol(m, position1, value1);
                                                    msg.invSymbol(m, position2, value2);
                                                    msg.invSymbol(m, position3, value3);
                                                    msg.invSymbol(m, position4, value4);

                                                    ErrorInfo errorInfo = (((uint64_t) position4)<<(7*m))
                                                                        | (((uint64_t) value4)<<(6*m))
                                                                        | (((uint64_t) position3)<<(5*m))
                                                                        | (((uint64_t) value3)<<(4*m))
                                                                        | (((uint64_t) position2)<<(3*m))
                                                                        | (((uint64_t) value2)<<(2*m))
                                                                        | (((uint64_t) position1)<<(m))
                                                                        | ((uint64_t) value1);
                                                    //ErrorInfo errorInfo;
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position1, (int) value1));
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position2, (int) value2));
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position3, (int) value3));
                                                    //errorInfo.insert(std::make_pair<int, int>((int) position4, (int) value4));
                                                    ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
                                                    if (result==SDC) return false;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return true;
    }
private:
    void genGenPoly() {
        gPoly.setCoeff(0, 0);   // 1

        GFPoly<p, m> temp(1);   // x + a^i
        temp.setCoeff(1, m);
        for (int i=0; i<symR; i++) {
            temp.setCoeff(0, i+m+1);
            gPoly *= temp;
        }

        //printf("Generator poly: %d %d\n", m, symR);
        //gPoly.print();
    }

    ErrorType decodeCorrectable(ECCWord *msg, ECCWord *decoded, ErrorInfo errorInfo) {
        // step 1: copy the message data
        //decoded->clone(msg);

        // step 2: generate syndrome
        bool synError = genSyndrome(msg);

        // Step 3: if all of syndrome bits are zero, the word can be assumed to be error free
        if (synError) {
            assert(m*symR <= 64);
            uint64_t syndromeKey = 0ull;
            for (int i=0; i<symR; i++) {
                syndromeKey ^= (syndrome[i].getValue() << (i*m));
            }
            auto it = correctableSyndromes.find(syndromeKey);
            if (it!=correctableSyndromes.end()) {
                return SDC;
            }
            correctableSyndromes.insert(std::make_pair<uint64_t, ErrorInfo>((uint64_t) syndromeKey, (ErrorInfo) errorInfo));
            return CE;
        } else {
            return SDC;
        }
    }

    // member fields
public:
    int symN, symK, symR, symT;
    int pos1, pos2, pos3;
    GFPoly<p, m> gPoly;
    GFElem<p, m>* syndrome;
    std::unordered_map<uint64_t, ErrorInfo> correctableSyndromes;
};


//GONG: RS dual decoder that can correct error and erasures concurrently
template <int p, int m>
class RS_DUAL : public Codec {        
public:
	RS_DUAL(const char *name, int _symN, int _symR, int _symB)
        : Codec(name, m*_symN, m*_symR){ 
		//: symN(_symN), symR(_symR), symB(_symB), symK(_symN-_symR), indexMax((p<<(m-1))-1){  
		symN = _symN;
		symR = _symR;
		symB = _symB; 
		symK = _symN-_symR;
		indexMax = (p<<(m-1))-1;
		syndrome = new GFElem<p,m> [symR];
		errata = new GFElem<p,m> [symR+symB];
		errata_raw = new GFElem<p,m> [symR+symB];
		erasure = new GFElem<p,m> [symR+symB];
		location = new int[symR];
		error = new GFElem<p,m> [symR];
		mu = new GFElem<p,m> [symR+symB]; //mu
		la = new GFElem<p,m> [symR+symB]; //lambda
		tmp_mu = new GFElem<p,m> [symR+symB]; //mu
		tmp_la = new GFElem<p,m> [symR+symB]; //lambda
		reg = new GFElem<p,m> [symR];
	};
	~RS_DUAL(){
		delete[] syndrome;		
		delete[] errata;		
		delete[] errata_raw;	
		delete[] erasure;		
		delete[] location;		
		delete[] error;		
		delete[] mu;		
		delete[] la;		
		delete[] tmp_mu;		
		delete[] tmp_la;		
		delete[] reg;		
	};
	bool hasSDC;
	int symN;
	int symK;
	int symR;
	int _L; //number of errors including erasure
	int symB; //number of erasures
	int indexMax;
	int* location;
	GFElem<p,m>* syndrome;
	GFElem<p,m>* erasure; //erasure polynomial
	GFElem<p,m>* errata;
	GFElem<p,m>* errata_raw;
	GFElem<p,m>* error;
	//variables for BM
	GFElem<p,m> de; //delta
	GFElem<p,m> ga; //gamma
	GFElem<p,m> *mu; //mu
	GFElem<p,m> *la; //lambda
	GFElem<p,m> *tmp_mu; //mu
	GFElem<p,m> *tmp_la; //lambda
	//variables for Chien	
	GFElem<p,m> *reg;

    ErrorType decode(ECCWord *msg, ECCWord *decoded, std::set<int>* correctedPos, std::list<int>* ErasureLocation) {
    //ErrorType decode(ECCWord *msg, ECCWord *decoded, std::list<int>* ErasureLocation) {
		bool synError = genSyndrome(msg);
		if(synError){
        	decoded->clone(msg);
			ErasurePolyGen(ErasureLocation);
			BM();
			if(2*_L+ErasureLocation->size()>symR){
				//printf("DUE from BM\n");
				//print_errata();
				return DUE;
			}
			
			if(!Chien()) {
				//printf("DUE from Chien\n");
				return DUE;
			}
			ErrorEval();
			hasSDC = false;
			Correction(decoded, correctedPos, ErasureLocation);
			//Correction(decoded);
		}else{
			//generally it might be SDC if we generated all zero syndromes
			//the below code is for the exception cases where errors exists only in parity (categorized as DUE)
			if(msg->isZero()) return CE;
			else return SDC;
		}
		
		if(decoded->isZero()){
			//if(_L+symB>10){
			//	msg->print();
			//	for(int i=0; i<_L+symB; i++){
			//		printf("location[%i]: %i\n", i, indexMax-location[i]);
			//		printf("error index: %i\n", error[i].getIndexValue());
			//	}
			//}	
			return CE;
		}
		else
		{ 
			if(hasSDC) return DUE;
/*
			printf("SDC1\n");
			msg->print();
			decoded->print();
			for(int i=0; i<_L+symB; i++){
				printf("location[%i]: %i\n", i, indexMax-location[i]);
				printf("error index: %i\n", error[i].getIndexValue());
			}
*/	
			return SDC;
		}
		
	}
	bool genSyndrome(ECCWord* msg){
		bool synError = false;
		GFElem<p,m> received[symN];
        for (int i=0; i<symN; i++) {
            received[i].setValue(msg->getSymbol(m, i));
			//printf("received[%i]: %i\n", i, received[i].getIndexValue());
        }
		for(int i=0; i<symR; i++){
			syndrome[i] = GFElem<p,m>(indexMax);//reinitialize
			for(int j=0; j<symK+symR; j++){
				if(j!=symK+symR-1){
					syndrome[i] = (syndrome[i] + received[symK+symR-1-j]) * (GFElem<p,m>(1) ^ (i+1));
				}else{
					syndrome[i] += received[0];		
				}
			}
			if(!syndrome[i].isZero()){
				synError = true;		
			}
		}	
		return synError;
	}
	void ErasurePolyGen(std::list<int> *ErasureLocation){
		if(ErasureLocation->size()==0){
			for(int i=0; i<symR+symB; i++) erasure[i] = GFElem<p,m>(indexMax);
			erasure[0] = GFElem<p,m>(0);
		}else{
			bool first = true;
			for(std::list<int>::iterator it=ErasureLocation->begin(); it!=ErasureLocation->end(); it++){
				if(first){
					erasure[0] = GFElem<p,m>(0);
					erasure[1] = GFElem<p,m>(*it);
					for(int i=2; i<symR+symB; i++) erasure[i] = GFElem<p,m>(indexMax);
					first = false;
				}else{
					GFElem<p,m> tmp[symR+symB];
					for(int i=0; i<symR+symB; i++){
						tmp[i] = GFElem<p,m>(*it) * erasure[i];
					}
					for(int i=1; i<symR+symB; i++){
						erasure[i] += tmp[i-1]; 
					}
				} 
			}
			//for(int i=0; i<symR+symB; i++) printf("erasure[%i]: %i\n", i, erasure[i]);
		}
	}
	void BM(){//a modified version (inversion-less) 
		//GFElem<p,m> de; //delta
		//GFElem<p,m> mu[symR+symB]; //mu
		//GFElem<p,m> la[symR+symB]; //lambda
		//GFElem<p,m> ga; //gamma
		//GFElem<p,m> tmp_mu[symR+symB]; //mu
		//GFElem<p,m> tmp_la[symR+symB]; //lambda
		
		int l=0;
		ga = GFElem<p,m>(0);
		for(int i=0; i<symR+symB; i++){
			mu[i] = GFElem<p,m>(indexMax);
			la[i] = GFElem<p,m>(indexMax);
		}

		for(int i=0; i<symR+symB; i++){
			mu[i] = la[i] = erasure[i];
		}
		
		for(int k=1; k<symR; k++){
			if(k>symR-symB) {
				break;
			}
			//update of delta
			de = GFElem<p,m>(indexMax);
			for(int j=0; j<=k+symB; j++) {	
				de += mu[j] * syndrome[k-j+symB-1];
			}
			//update of mu
			for(int j=0; j<symR+symB; j++){
				if(j==0) tmp_mu[j] = ga * mu[j];
				else tmp_mu[j] = ga * mu[j] + de * la[j-1];
			}
			//update of lambda
			if(de.getIndexValue() != indexMax && 2*l <= k-1){
				for(int j=0; j<symR+symB; j++) tmp_la[j] = mu[j];	
			}else{
				for(int j=0; j<symR+symB; j++) {
					if(j==0) tmp_la[j] = GFElem<p,m>(indexMax);
					else tmp_la[j] = la[j-1];
				}
			}
			//update of length and gammah
			if(de.getIndexValue() != indexMax && 2*l <= k-1) {
				l = k - l;
				ga = de; 
			}
				
			for(int j=0; j<symR+symB; j++) {
				mu[j] = tmp_mu[j];
				la[j] = tmp_la[j];
			}
		}
		
		//printf("_L: %i\n", l);
		_L=l;
		for(int i=0; i<symR+symB; i++) {
			errata[i] = mu[i] / mu[0];
			errata_raw[i] = mu[i];
		}
	}
	bool Chien(){ //errata location
		//printf("L+S: %i\n", _L+symB);
		//GFElem<p,m> reg[_L+symB];
		int cnt=0;
		for(int i=0; i<indexMax; i++){
			GFElem<p,m> sum = GFElem<p,m>(indexMax);		
			//for(int j=0; j<=_L+symB; j++){
			for(int j=0; j<=_L+symB; j++){
				if(i==0) reg[j] = errata[j+1];
				else reg[j] = reg[j] * (GFElem<p,m>(1)^(j+1));
				sum += reg[j];
			}
			//printf("sum: %i\n", sum.getIndexValue());
			if((i==0 || i >= indexMax-symN) && sum == GFElem<p,m>(0)){
				location[cnt] = i;
				//printf("locaton[%i] : %i \n", cnt, indexMax-location[cnt]);
				cnt++;
			}
		}
		if(cnt!=_L+symB) return false;
		else return true;

	}
	void ErrorEval(){
		GFElem<p,m> poly[symR+_L+symB];
		for(int i=0; i<symR+_L+symB; i++) poly[i] = GFElem<p,m>(indexMax);
		for(int i=0; i<symR; i++){
			for(int j=0; j<=_L+symB; j++){
				poly[i+j] += syndrome[i] * errata_raw[j];//errata_raw[j];
			}		
		}
		
		GFElem<p,m> numer[_L+symB];
		GFElem<p,m> denom[_L+symB];
		for(int i=0; i<_L+symB; i++){
			numer[i] = GFElem<p,m>(indexMax);
			denom[i] = GFElem<p,m>(indexMax);
			for(int j=0; j<symR; j++){
				numer[i] += poly[j] * (GFElem<p,m>(location[i]) ^ (j+1) );
			}	
			for(int j=0; j<symR; j++){
				if(j%2==1) {
					denom[i] += errata_raw[j] * (GFElem<p,m>(location[i]) ^ (j-1)) * GFElem<p,m>(location[i]);	 
				}
			}
			//printf("num: %i den: %i\t", numer[i].getIndexValue(), denom[i].getIndexValue());
			error[i] = numer[i] / denom[i];
			//printf("error [%i]: %2x\n", i, error[i].getPolyValue());
		}
	}
	void Correction(ECCWord* decoded, std::set<int>* correctedPos, std::list<int>* ErasureLocation){
	//void Correction(ECCWord* decoded){
		for(int i=0; i<_L+symB; i++){
			int symID =  (indexMax-location[i])%indexMax;
			int e_index =  error[i].getIndexValue()+1;
				//assuming only inherent faults corrected as "errors"
				//chip fault erasure locations (or parity-related symbols) should be put in advance
				std::list<int>::iterator it = std::find(ErasureLocation->begin(), ErasureLocation->end(), symID);
				if(it== ErasureLocation->end()){
					//printf("symID: %i\t error(index): %i\n", symID, e_index);
					if(e_index != 0 ||
					   e_index != 1 ||
					   e_index != 2 ||
					   e_index != 4) hasSDC = true;
				}
			insertCorrectionInfo(symID, e_index);
			decoded->invSymbol(m,symID, e_index);
            if(correctedPos!=NULL) {
                correctedPos->insert(symID);
            }
		}
	}
	void init(){
		for(int i=0; i<symR; i++){
			syndrome[i] = GFElem<p,m>(indexMax);		
			error[i] = GFElem<p,m>(indexMax);		
			reg[i] = GFElem<p,m>(indexMax);		
			location[i] = -1;		
		}	
		for(int i=0; i<symR+symB; i++){
			errata[i] = GFElem<p,m>(indexMax);		
			errata_raw[i] = GFElem<p,m>(indexMax);		
			erasure[i] = GFElem<p,m>(indexMax);		
			mu[i] = GFElem<p,m>(indexMax);		
			la[i] = GFElem<p,m>(indexMax);		
			tmp_mu[i] = GFElem<p,m>(indexMax);		
			tmp_la[i] = GFElem<p,m>(indexMax);		
		}
	}
	void print_syndrome(){
		printf("syndrome (index): \n");
		for(int i=0; i<symR; i++){
			printf("%i\t%i\n", i, syndrome[i].getIndexValue());	
		}
	}
	void print_errata(){
		printf("errata (index): \n");
		for(int i=0; i<symR+symB; i++){
			printf("%i\t%i\n", i, errata[i].getIndexValue());	
		}
	}
	//dummy definitions
    void encode(Block *data, ECCWord *encoded) {}
    ErrorType decode(ECCWord *msg, ECCWord *decoded, std::set<int>* correctedPos) {}
};

#endif /* __RS_HH__ */
