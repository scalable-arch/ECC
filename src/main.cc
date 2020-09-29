#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <random>

#include "Config.hh"
#include "FaultRateInfo.hh"

#include "DomainGroup.hh"
#include "Tester.hh"
#include "Scrubber.hh"

#include "prior.hh"
#include "Bamboo.hh"
#include "Frugal.hh"
#include "AIECC.hh"
#include "XED.hh"
#include "REGB.hh"
#include "AccessFACH.hh"
#include "DUO.hh"

void testBamboo(int ID, DomainGroup *dg);
void testFrugal(int ID);
void testAIECC(int ID);
void testDUO(int ID);

#define BAMBOO
//#define AGECC

int main(int argc, char **argv)
{
    if (argc<5) {
        printf("Usage: %s ECCID runCnt RandomSeed FaultType1 FaultType2 ...\n", argv[0]);
        exit(1);
    }

    // random seed
    srand(atoi(argv[3]));
    //srand(time(NULL));

    char filePrefix[256];
    DomainGroup *dg = NULL;
    ECC *ecc = NULL;
    Tester *tester = NULL;
    Scrubber *scrubber = NULL;
    
    //int DIMMcnt = 100000;
    int DIMMcnt = 4;
    //int DIMMcnt = 2;
    //int DIMMcnt = 1;

    initCache(); // Fault cache init

#ifdef BAMBOO
    switch (atoi(argv[1])) {
        // 2 rank / x4 chip / 64-/72-bit channel
        case 0: // bit-level
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 8);
            ecc = new ECCNone();
            sprintf(filePrefix, "000.4x16.None.%s", argv[3]);
            break;
        case 1:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new SECDED72b();
            sprintf(filePrefix, "001.4x18.SECDED72b.%s", argv[3]);
            break;
        case 2:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 17, 4, 8);
            ecc = new SPC66bx4();
            sprintf(filePrefix, "002.4x17.SPC66bx4.%s", argv[3]);
            break;
        case 3:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 17, 4, 8);
            ecc = new SPCTPD68bx4();
            sprintf(filePrefix, "003.4x17.SPCTPD68bx4.%s", argv[3]);
            break;
        case 10:    // chip-level
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            sprintf(filePrefix, "010.4x18.AMD.%s", argv[3]);
            break;
        case 11:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b();
            sprintf(filePrefix, "011.4x18.QPC.%s", argv[3]);
            break;
        case 20:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 36, 4, 8);
            ecc = new AMDDChipkill144b();
            sprintf(filePrefix, "020.4x36.DAMD.%s", argv[3]);
            break;
        case 21:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 36, 4, 8);
            ecc = new OPC144b();
            sprintf(filePrefix, "021.4x36.OPC.%s", argv[3]);
            break;
        // 2 rank / x8 chip / 72-bit channel
        case 30:    // bit-level
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8, 8);
            ecc = new SECDED72b();
            sprintf(filePrefix, "030.9x8.SECDED72b.%s", argv[3]);
            break;
        // 2 rank / x8 chip / 144-bit channel
        case 40:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 8, 8);
            ecc = new S8SC144b();
            sprintf(filePrefix, "040.8x18.S8SC.%s", argv[3]);
            break;
        case 41:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 8, 8);
            ecc = new OPC144b();
            sprintf(filePrefix, "041.8x18.OPC.%s", argv[3]);
            break;
        case 50:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,1);
            sprintf(filePrefix, "050.4x18.QPC41.%s", argv[3]);
            break;
        case 51:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,2);
            sprintf(filePrefix, "051.4x18.QPC42.%s", argv[3]);
            break;
        case 52:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,3);
            sprintf(filePrefix, "052.4x18.QPC43.%s", argv[3]);
            break;
        case 53:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,4);
            sprintf(filePrefix, "053.4x18.QPC44.%s", argv[3]);
            break;
        case 60:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 19, 4, 8);
            ecc = new QPC76b();
            sprintf(filePrefix, "060.19x4.QPC76b.%s", argv[3]);
            break;
        default:
            printf("Invalid ECC ID\n");
            exit(1);
    }
#endif
#ifdef FRUGAL_ECC
    switch (atoi(argv[1])) {
        // 2 rank / x8 chip / 64-bit channel
        case 0:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 8, 8);
            ecc = new ECCNone();
            sprintf(filePrefix, "000.8x8.None.%s", argv[3]);
            break;
        case 1:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 8, 8);
            ecc = new FrugalECC64bMultix8();
            sprintf(filePrefix, "001.8x8.FECC+Multi.%s", argv[3]);
            break;
        // 2 rank / x8 chip / 72-bit channel
        case 10:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new SECDED72b();
            sprintf(filePrefix, "010.9x8.SECDED72b.%s", argv[3]);
            break;
        case 11:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new S8SC72b();
            sprintf(filePrefix, "011.9x8.S8SC.%s", argv[3]);
            break;
        case 12:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new OPC72b();
            sprintf(filePrefix, "012.9x8.OPC.%s", argv[3]);
            break;
        case 13:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new LOTECC();
            sprintf(filePrefix, "013.9x8.OTECC.%s", argv[3]);
            break;
        case 14:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new MultiECC();
            sprintf(filePrefix, "014.9x8.MultiECC.%s", argv[3]);
            break;
        case 15:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new FrugalECC72bNoEFP();
            sprintf(filePrefix, "015.9x8.FECCnoEFP.%s", argv[3]);
            break;
        case 16:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 8);
            ecc = new FrugalECC72bOPC();
            sprintf(filePrefix, "016.9x8.FECC+OPC.%s", argv[3]);
            break;
        // 2 rank / x8 chip / 80-bit channel
        case 20:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 10, 8);
            ecc = new S8SC80b();
            sprintf(filePrefix, "020.10x8.S8SC.%s", argv[3]);
            break;
        case 21:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 10, 8);
            ecc = new OPC80b();
            sprintf(filePrefix, "021.10x8.OPC.%s", argv[3]);
            break;
        // 2 rank / x8 chip / 128-bit channel
        case 30:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 16, 8);
            ecc = new VECC128bx8();
            sprintf(filePrefix, "030.16x8.VECC.%s", argv[3]);
            break;
        // 2 rank / x8 chip / 144-bit channel
        case 41:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 8);
            ecc = new OPC144b();
            sprintf(filePrefix, "040.18x8.OPC.%s", argv[3]);
            break;
        case 41:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 8);
            ecc = new VECC144bx8();
            sprintf(filePrefix, "041.18x8.VECC.%s", argv[3]);
            break;


        case 4:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new AMDChipkill64b();
            sprintf(filePrefix, "02.4x16.AMD.%s", argv[3]);
            break;
        case 2:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new QPC64b();
            sprintf(filePrefix, "4x16.02.QPC.%s", argv[3]);
            break;
        case 11:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new VECC64bAMD();
            sprintf(filePrefix, "4x16.11.VECC+AMD.%s", argv[3]);
            break;
        case 12:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new VECC64bQPC();
            sprintf(filePrefix, "4x16.12.VECC+QPC.%s", argv[3]);
            break;
        case 13:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new VECC64bS8SCD8SD();
            sprintf(filePrefix, "4x16.13.VECC+S8SCD8SD.%s", argv[3]);
            break;
        case 14:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new VECC64bMultix4();
            sprintf(filePrefix, "4x16.14.VECC+Multi.%s", argv[3]);
            break;
        case 20:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new FrugalECC64bNoEFP();
            sprintf(filePrefix, "4x16.20.FECC+noEFP.%s", argv[3]);
            break;
        case 21:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new FrugalECC64b();
            sprintf(filePrefix, "4x16.21.FECC+QPC.%s", argv[3]);
            break;
        case 22:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new FrugalECC64b2();
            sprintf(filePrefix, "4x16.22.FECC2+QPC.%s", argv[3]);
            break;
        case 23:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new FrugalECC64b3();
            sprintf(filePrefix, "4x16.23.FECC3+QPC.%s", argv[3]);
            break;
        case 24:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4);
            ecc = new FrugalECC64bMultix4();
            sprintf(filePrefix, "4x16.24.FECC+Multi.%s", argv[3]);
            break;
        // 2 rank / x4 chip / 68-bit channel
        // 2 rank / x4 chip / 72-bit channel
        case 200:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4);
            ecc = new SECDED72b();
            sprintf(filePrefix, "4x18.00.SECDED72b.%s", argv[3]);
            break;
        case 201:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4);
            ecc = new AMDChipkill72b();
            sprintf(filePrefix, "4x18.01.AMD.%s", argv[3]);
            break;
        case 202:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4);
            ecc = new QPC72b();
            sprintf(filePrefix, "4x18.02.QPC.%s", argv[3]);
            break;
        case 210:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4);
            ecc = new COP();
            sprintf(filePrefix, "4x18.10.COP.%s", argv[3]);
            break;
        case 211:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4);
            ecc = new COPER();
            sprintf(filePrefix, "4x18.11.COPER.%s", argv[3]);
            break;
        // 2 rank / x4 chip / 128-bit channel
        case 500:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 32, 4);
            ecc = new VECC128bx4();
            sprintf(filePrefix, "4x32.00.VECC.%s", argv[3]);
            break;
        // 2 rank / x4 chip / 136-bit channel
        case 600:
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 34, 4);
            ecc = new VECC136bx4();
            sprintf(filePrefix, "4x34.00.VECC.%s", argv[3]);
            break;
        //// 2 rank / x4 chip / 144-bit channel
        //case 700:
        //    dg = new DomainGroupDDR(DIMMcnt/4, 2, 36, 4);
        //    ecc = new S4SCD4SD144b();
        //    sprintf(filePrefix, "4x36.00.S4SCD4SD.%s", argv[3]);
        //    break;


        default:
            printf("Invalid ECC ID\n");
            exit(1);
    }
#endif
#ifdef AIECC
    switch (atoi(argv[1])) {
        case 0:
            dg = new DomainGroupDDRCA(DIMMcnt/4, 2, 18, 4, 3.7);
            ecc = new DDR4QPC72b();
            sprintf(filePrefix, "000.18x4.DDR4.%s", argv[3]);
            break;
        case 1:
            dg = new DomainGroupDDRCA(DIMMcnt/4, 2, 18, 4, 3.7);
            ecc = new AzulQPC72b();
            sprintf(filePrefix, "001.18x4.Azul.%s", argv[3]);
            break;
        case 2:
            dg = new DomainGroupDDRCA(DIMMcnt/4, 2, 18, 4, 3.7);
            ecc = new AIECCQPC72b();
            sprintf(filePrefix, "002.18x4.AIECC.%s", argv[3]);
            break;
        case 3:
            dg = new DomainGroupDDRCA(DIMMcnt/4, 2, 18, 4, 3.7);
            ecc = new NickQPC72b();
            sprintf(filePrefix, "003.18x4.IBM.%s", argv[3]);
            break;
        default:
            printf("Invalid ECC ID\n");
            exit(1);
    }
#endif
#ifdef AGECC
    switch (atoi(argv[1])) {
        case 0:     // None
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 8);
            ecc = new ECCNone();
            sprintf(filePrefix, "000.4x16.None.%s", argv[3]);
            break;
        case 1:     // None
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new ECCNone();
            sprintf(filePrefix, "001.4x18.None.%s", argv[3]);
            break;
        case 2:     // Zero EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new SECDED72b();
            sprintf(filePrefix, "002.4x18.SECDED72b.%s", argv[3]);
            break;
        case 9:     // Zero EGB + post-processing
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            sprintf(filePrefix, "009.4x18.AMD.%s", argv[3]);
            break;
        case 10:     // Zero EGB + no post-processing
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(false);
            sprintf(filePrefix, "010.4x18.AMD2.%s", argv[3]);
            break;
        case 20:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,1);
            sprintf(filePrefix, "020.4x18.QPC41.%s", argv[3]);
            break;
        case 21:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,2);
            sprintf(filePrefix, "021.4x18.QPC42.%s", argv[3]);
            break;
        case 22:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,3);
            sprintf(filePrefix, "022.4x18.QPC43.%s", argv[3]);
            break;
        case 23:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(4,4);
            sprintf(filePrefix, "023.4x18.QPC44.%s", argv[3]);
            break;
        case 24:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(3,3);
            sprintf(filePrefix, "024.4x18.QPC33.%s", argv[3]);
            break;
        case 25:     // EGB
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72b(2,2);
            sprintf(filePrefix, "025.4x18.QPC22.%s", argv[3]);
            break;
        case 100:   // on-chip ECC
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 18);
            ecc = new OnChip64b();
            sprintf(filePrefix, "100.4x16.OnChip.%s", argv[3]);
            break;
        case 110:   // on-chip ECC + SEC-DED
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bSECDED();
            sprintf(filePrefix, "110.4x18.OnChip+SECDED.%s", argv[3]);
            break;
        case 111:   // on-chip ECC + AMD (w/ postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(true);
            sprintf(filePrefix, "111.4x18.OnChip+AMD.%s", argv[3]);
            break;
        case 112:   // on-chip ECC + AMD (w/o postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(false);
            sprintf(filePrefix, "112.4x18.OnChip+AMD2.%s", argv[3]);
            break;
        case 113:   // on-chip ECC + QPC
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bQPC72b(4,2);
            sprintf(filePrefix, "113.4x18.OnChip+QPC42.%s", argv[3]);
            break;
        case 114:   // on-chip ECC + QPC
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bQPC72b(4,3);
            sprintf(filePrefix, "114.4x18.OnChip+QPC43.%s", argv[3]);
            break;
        case 115:   // on-chip ECC + QPC
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bQPC72b(4,4);
            sprintf(filePrefix, "115.4x18.OnChip+QPC44.%s", argv[3]);
            break;
        case 116:   // on-chip ECC + QPC
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bQPC72b(3,3);
            sprintf(filePrefix, "116.4x18.OnChip+QPC33.%s", argv[3]);
            break;
        case 117:   // on-chip ECC + QPC
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bQPC72b(2,2);
            sprintf(filePrefix, "117.4x18.OnChip+QPC22.%s", argv[3]);
            break;
        case 130:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(true);    // w/ fault diagnosis
            sprintf(filePrefix, "130.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 132:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(false);   // w/o fault diagnosis
            sprintf(filePrefix, "132.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 140:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, true);     // w/ retire
            sprintf(filePrefix, "140.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 141:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, false);    // w/o retire
            sprintf(filePrefix, "141.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 200:       // Zero EGB + post-processing
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            sprintf(filePrefix, "200.4x18.AMD.%s", argv[3]);
            break;
        case 201:       // Zero EGB + post-processing
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "201.4x18.AMD.%s", argv[3]);
            break;
        case 202:       // Zero EGB + post-processing
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            ecc->setMaxRetiredBlkCount(64);
            sprintf(filePrefix, "202.4x18.AMD.%s", argv[3]);
            break;
        case 203:       // Zero EGB + post-processing
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            ecc->setMaxRetiredBlkCount(2048);
            sprintf(filePrefix, "203.4x18.AMD.%s", argv[3]);
            break;
        case 204:       // Zero EGB + post-processing
            dg = new DomainGroupDDR(DIMMcnt/4, 2, 18, 4, 8);
            ecc = new AMDChipkill72b(true);
            ecc->setMaxRetiredBlkCount(16384);
            sprintf(filePrefix, "204.4x18.AMD.%s", argv[3]);
            break;
        case 210:   // on-chip ECC + AMD (w/ postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(true);
            sprintf(filePrefix, "210.4x18.OnChip+AMD.%s", argv[3]);
            break;
        case 211:   // on-chip ECC + AMD (w/ postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(true);
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "211.4x18.OnChip+AMD.%s", argv[3]);
            break;
        case 212:   // on-chip ECC + AMD (w/ postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(true);
            ecc->setMaxRetiredBlkCount(64/2);
            sprintf(filePrefix, "212.4x18.OnChip+AMD.%s", argv[3]);
            break;
        case 213:   // on-chip ECC + AMD (w/ postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(true);
            ecc->setMaxRetiredBlkCount(2048/2);
            sprintf(filePrefix, "213.4x18.OnChip+AMD.%s", argv[3]);
            break;
        case 214:   // on-chip ECC + AMD (w/ postprocessing)
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new OnChip72bAMD(true);
            ecc->setMaxRetiredBlkCount(16384/2);
            sprintf(filePrefix, "214.4x18.OnChip+AMD.%s", argv[3]);
            break;
        case 220:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new XED_DDDC(true);    // w/ fault diagnosis
            sprintf(filePrefix, "220.4x18.XED_DDDC.%s", argv[3]);
            break;
        case 221:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new XED_DDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "221.4x18.XED_DDDC.%s", argv[3]);
            break;
        case 222:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new XED_DDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(64/2);
            sprintf(filePrefix, "222.4x18.XED_DDDC.%s", argv[3]);
            break;
        case 223:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new XED_DDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(2048/2);
            sprintf(filePrefix, "223.4x18.XED_DDDC.%s", argv[3]);
            break;
        case 224:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 18);
            ecc = new XED_DDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(16384/2);
            sprintf(filePrefix, "224.4x18.XED_DDDC.%s", argv[3]);
            break;
        case 230:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, true);     // w/ retire
            ecc->setDoRetire(false);
            sprintf(filePrefix, "230.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 231:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, true);     // w/ retire
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "231.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 232:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, true);     // w/ retire
            ecc->setMaxRetiredBlkCount(64);
            sprintf(filePrefix, "232.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 233:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, true);     // w/ retire
            ecc->setMaxRetiredBlkCount(2048);
            sprintf(filePrefix, "233.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 234:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 18, 4, 8);
            ecc = new QPC72bREGB(true, true);     // w/ retire
            ecc->setMaxRetiredBlkCount(16384);
            sprintf(filePrefix, "234.4x18.QPC_REGB.%s", argv[3]);
            break;
        case 240:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(true);    // w/ fault diagnosis
            sprintf(filePrefix, "240.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 241:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "241.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 242:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(64/2);
            sprintf(filePrefix, "242.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 243:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(2048/2);
            sprintf(filePrefix, "243.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 244:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 9, 4, 18);
            ecc = new XED_SDDC(true);    // w/ fault diagnosis
            ecc->setMaxRetiredBlkCount(16384/2);
            sprintf(filePrefix, "244.4x9.XED_SDDC.%s", argv[3]);
            break;
        case 300:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(1);
            sprintf(filePrefix, "300.4x16.DUO.%s", argv[3]);
            break;
        case 301:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(1);
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "301.4x16.DUO.%s", argv[3]);
            break;
        case 302:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(1);
            ecc->setMaxRetiredBlkCount(64);
            sprintf(filePrefix, "302.4x16.DUO.%s", argv[3]);
            break;
        case 303:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(1);
            ecc->setMaxRetiredBlkCount(2048);
            sprintf(filePrefix, "303.4x16.DUO.%s", argv[3]);
            break;
        case 304:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(1);
            ecc->setMaxRetiredBlkCount(16384);
            sprintf(filePrefix, "304.4x16.DUO.%s", argv[3]);
            break;
        case 310:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(2);
            sprintf(filePrefix, "310.4x16.DUO.%s", argv[3]);
            break;
        case 311:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(2);
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "311.4x16.DUO.%s", argv[3]);
            break;
        case 312:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(2);
            ecc->setMaxRetiredBlkCount(64);
            sprintf(filePrefix, "312.4x16.DUO.%s", argv[3]);
            break;
        case 313:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(2);
            ecc->setMaxRetiredBlkCount(2048);
            sprintf(filePrefix, "313.4x16.DUO.%s", argv[3]);
            break;
        case 314:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(2);
            ecc->setMaxRetiredBlkCount(16384);
            sprintf(filePrefix, "314.4x16.DUO.%s", argv[3]);
            break;
        case 320:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(3);
            sprintf(filePrefix, "320.4x16.DUO.%s", argv[3]);
            break;
        case 321:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(3);
            ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "321.4x16.DUO.%s", argv[3]);
            break;
        case 322:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(3);
            ecc->setMaxRetiredBlkCount(64);
            sprintf(filePrefix, "322.4x16.DUO.%s", argv[3]);
            break;
        case 323:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(3);
            ecc->setMaxRetiredBlkCount(2048);
            sprintf(filePrefix, "323.4x16.DUO.%s", argv[3]);
            break;
        case 324:
            dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
            ecc = new DUO64bx4(3);
            ecc->setMaxRetiredBlkCount(16384);
            sprintf(filePrefix, "324.4x16.DUO.%s", argv[3]);
            break;
		case 330: 
            dg = new DomainGroupDDR(DIMMcnt/4, 4, 9, 4, 17);
            ecc = new DUO36bx4(6,false,false,0);
			//ecc->setDoRetire(false);//no retirement
            //ecc->setMaxRetiredBlkCount(0);
            sprintf(filePrefix, "330.4x9(BL17).DUO.%s", argv[3]);
            break;
		case 331:
            dg = new DomainGroupDDR(DIMMcnt/4, 4, 9, 4, 17);
            //ecc = new DUO36bx4(3);
            ecc = new DUO36bx4(6,false,true,2048);
            //ecc->setMaxRetiredBlkCount(2048);
            sprintf(filePrefix, "331.4x9(BL17).DUO.%s", argv[3]);
            break;
        default:
            printf("Invalid ECC ID\n");
            exit(1);
    }
#endif /* AGECC */

    // system-level test:
    // If the 4th argument is 'S', it will use fault rate to generate a fault
    // and type the fault based on the fault breakdown ratios (single-bit, single-row, ...)
    if (strcmp(argv[4], "S")==0) {
        tester = new TesterSystem();
        scrubber = new PeriodicScrubber(8);

        string faults[argc-5];
        for (int i=5; i<argc; i++) {
            faults[i-5] = string(argv[i]);
        }
        tester->test(dg, ecc, scrubber, atol(argv[2]), filePrefix, argc-5, faults);
        delete tester;
        delete scrubber;
    }
    else
    // Scenario-based test
    // If the 4th argument is not 'S', it will use an individual scenario (E.g. single bit + single bit faults on a single ECC block)
    {
        tester = new TesterScenario();
        scrubber = new NoScrubber();
        
        string faults[argc-4];
        for (int i=4; i<argc; i++) {
            faults[i-4] = string(argv[i]);
        }
        tester->test(dg, ecc, scrubber, atol(argv[2]), filePrefix, argc-4, faults);
        delete tester;
        delete scrubber;
    }
}

