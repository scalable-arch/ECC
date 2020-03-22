#include "DUO.hh"
#include "FaultDomain.hh"
#include "hsiao.hh"

//------------------------------------------------------------------------------
DUO64bx4::DUO64bx4(int _maxPin) : ECC(PIN9), maxPin(_maxPin) {
    codec = new RS<2, 9>("3.5PC\t16\t4\t", 64, 7, maxPin);
}

ErrorType DUO64bx4::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    ECCWord msg = {codec->getBitN(), codec->getBitK()};
    ECCWord decoded = {codec->getBitN(), codec->getBitK()};

    bool synError;
    bool parity, parityError;
    ErrorType result;

    if (errorBlk.isZero()) {
        return NE;
    }

    // Up to 3-sym corrections
    msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());

    result = codec->decode(&msg, &decoded, &correctedPosSet);

    if ((result==CE)||(result==SDC)) {
        parity = 0;
        for (int i=0; i<15; i++) {
            parity ^= decoded.getBit(36*i);
        }
        parityError = (parity!=decoded.getBit(512));

        if (parityError) {
            correctedPosSet.clear();
            return DUE;
        }
    } else if (result==DUE) {
        // burst error correction
        result = codec->decodeBurstDUO64bx4(&msg, &decoded, 4, &correctedPosSet);

        if ((result==CE)||(result==SDC)) {
            parity = 0;
            for (int i=0; i<15; i++) {
                parity ^= decoded.getBit(36*i);
            }
            parityError = (parity!=decoded.getBit(512));

            if (parityError) {
                correctedPosSet.clear();
                return DUE;
            }
        }
    } else {
        assert(0);  // NE
    }

    return result;
}

unsigned long long DUO64bx4::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        double goodSymProb = pow(1-cellFaultRate, fd->getBeatHeight());
        double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) + fd->getChannelWidth()*pow(goodSymProb, fd->getChannelWidth()-1)*(1-goodSymProb);

        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/64;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}





//------------------------------------------------------------------------------
//GONG

DUO36bx4::DUO36bx4(int _maxPin, bool _doPostprocess, bool _doRetire, int _maxRetiredBlkCount)
	: ECC(PIN17, _doPostprocess, _doRetire, _maxRetiredBlkCount), maxPin(_maxPin) {
		
    codec = new RS<2, 8>("1.5PC\t16\t4\t", 76, 12, maxPin, 9);
	rs_dual_8 = new RS_DUAL<2,8>("8 symbol erasure + 2 symbol error", 76, 12, 8); 		
	rs_dual_10 = new RS_DUAL<2,8>("8 symbol erasure + 2 symbol error", 76, 12, 10); 		
	ErasureLocation = new std::list<int>;
}

ErrorType DUO36bx4::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
	ECCWord msg = {codec->getBitN(), codec->getBitK()};
    ECCWord decoded = {codec->getBitN(), codec->getBitK()};

    bool synError;
    ErrorType result;
	bool parityError = false;
    if (errorBlk.isZero()) {
        return NE;
    }

    // RS correction up to 6 symbols
    msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());

	result = codec->decode(&msg, &decoded, &correctedPosSet);
	//parity check
    if ((result==CE)||(result==SDC)) {
		//ParityCheck might not be requried
		//parityError = ParityCheck(&decoded, &errorBlk);	
		//if(parityError){
		//	//printf("Parity corrupted\n");
		//	//msg.print();
		//	//errorBlk.print();
		//	result = DUE_Parity;		
		//}			
		if(result==SDC){
			//case that error exists in parity
			if(decoded.isZero()) result = CE;
			else {
				printf("SDC from RS correction\n");
				msg.print();
				decoded.print();
			}
		}
    }

			
	//burst erasure and error correction
#ifdef DUE_BREAKDOWN
	if(result==DUE || result==DUE_Parity) {
#else
	if(result==DUE) {
#endif
		ErrorType tmp_result = result;
		int faultyChip = fd->FaultyChipDetect();
		if(faultyChip != -1){
			int startPos = ceil(faultyChip * 8.5); 
			//printf("chipID: %d\tstartPos: %d\n", chip->getChipID(), startPos);

        	ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
        	ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
			tmp_msg.clone(&msg);
			ErasureLocation->clear();
			//correct one symbol by using 4bit parity 
			CorrectByParity(&tmp_msg,&errorBlk,faultyChip);
			//set 8 symbols of each x4 chip as ersure symbols 
			for(int j=startPos; j<startPos+8; j++){
				ErasureLocation->push_back(j);
			}
			std::set<int> tmp_correctedPos;

			tmp_result = rs_dual_8->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos, ErasureLocation);
			
			//parity check
			if(tmp_result==CE){
				//ParityCheck might not be requried
				//bool parityError_ = ParityCheck(&tmp_decoded, &errorBlk);	
				//if(!parityError_){// || faultyChip==8){
				//	decoded.clone(&tmp_decoded);
				//	//why need this? see in needRetire()
				//	if(tmp_correctedPos.size()>11) {
				//			//printf("%i\n", tmp_correctedPos.size());
				//			//for(std::list<Fault*>::iterator it = fd->operationalFaultList.begin();
				//			//	it != fd->operationalFaultList.end();it++){
				//			//	(*it)->print();		
				//			//}
				//	}
					assert(tmp_correctedPos.size()<=11);
					this->correctedPosSet.clear();
					//erasure correction information
					//for(int j=startPos; j<startPos+rs_dual->symB-1; j++){
					//	this->correctedPosSet.insert(j);
					//}
					//error correction information
					for(std::set<int>::iterator it = tmp_correctedPos.begin(); it!=tmp_correctedPos.end(); ++it){
						this->correctedPosSet.insert(*it);
					}
					return CE;
				//}else{
				//	printf("parity corrputed\n");
				//	return DUE_Parity;
				//}	
			}else if(tmp_result==SDC){
				//printf("SDC: \n");					
				//msg.print();
				//tmp_decoded.print();	
				return SDC;
			}else{
				//tmp_result== DUE;
				//printf("DUE : uncorrecatble\n");
				//set two additinoal erasure positions (consequently 10 symbols erasure correction)
				//probably faults exist in parity-correction (CorrectByParity) related symbols
				std::set<int> erasure_set;
				erasure_set.insert(8);
				erasure_set.insert(25);
				erasure_set.insert(42);
				erasure_set.insert(59);

				ErasureLocation->clear();
				for(int j=startPos; j<startPos+8; j++){
					ErasureLocation->push_back(j);
				}
				
				if(faultyChip==0 || faultyChip==1) 	{
					ErasureLocation->push_back(8);
					erasure_set.erase(8);
				}else if(faultyChip==2 || faultyChip==3) {
					ErasureLocation->push_back(25);
					erasure_set.erase(25);
				}else if(faultyChip==4 || faultyChip==5) {
					ErasureLocation->push_back(42);
					erasure_set.erase(42);
				}else if(faultyChip==6 || faultyChip==7) {
					ErasureLocation->push_back(59);
					erasure_set.erase(59);
				}
				
				//try all three combinations
				for(std::set<int>::iterator it = erasure_set.begin(); it != erasure_set.end(); it++){
					tmp_correctedPos.clear();	
					ErasureLocation->push_back(*it);			
					tmp_result = rs_dual_10->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos, ErasureLocation);
					if(tmp_result==CE || tmp_result==SDC) break;
					else{
						ErasureLocation->pop_back();	
					}
				}

				if(tmp_result==CE || tmp_result==SDC) return tmp_result;
				else return DUE;
			}
		}//if Erasure information is available?
		else
		{
			//printf("DUE : no erasure available\n");
			//msg.print();
			//for(std::list<Fault*>::iterator it = fd->operationalFaultList.begin();
			//	it != fd->operationalFaultList.end();it++){
			//	//if( !(*it)->getIsSingleBeat() )

			//		printf("Chip %i overlapped?: %i\t", (*it)->getChipID(), (*it)->overlap(*(fd->operationalFaultList.begin())));
			//		(*it)->print();
			//	//}
			//}
#ifdef DUE_BREAKDOWN
			result = DUE_NoErasure;
#else
			result = DUE;
#endif
		}
		//Reaching here implies the ECC Word cannot be corrected or SDC.
		if(result==SDC){
			//msg.print();
			//for(std::list<Fault*>::iterator it = fd->operationalFaultList.begin();
			//	it != fd->operationalFaultList.end();it++){
			//		printf("chip ID: %d\t", (*it)->getChipID());(*it)->print();		
			//}
#ifdef DUE_BREAKDOWN
			return SDC_Erasure;
#else
			return SDC;
#endif
		}
		else
		{
			assert(result==DUE);
			return result;					
		}
	}
	assert(result==CE||result==SDC);
	
    return result;
}

unsigned long long DUO36bx4::getInitialRetiredBlkCount(FaultDomain *fd, Fault *fault) {
    double cellFaultRate = fault->getCellFaultRate();
    if (cellFaultRate==0) {
        return 0;
    } else {
        double goodSymProb = pow(1-cellFaultRate, fd->getBeatHeight());
        double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) + fd->getChannelWidth()*pow(goodSymProb, fd->getChannelWidth()-1)*(1-goodSymProb);

        unsigned long long totalBlkCount = ((MRANK_MASK^DEFAULT_MASK)+1)*fd->getChipWidth()/64;
        std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
        unsigned long long goodBlkCount = distribution(randomGenerator);
        unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
        return badBlkCount;
    }
}

bool DUO36bx4::ParityCheck(ECCWord* decoded, Block* errorBlk){
    bool parity0, parity1, parity2, parity3, parityError;
	parity0 = parity1 = parity2 = parity3 = false;
	for (int i=0; i<9*17; i++) {
        parity0 ^= decoded->getBit(4*i+0);
        parity1 ^= decoded->getBit(4*i+1);
        parity2 ^= decoded->getBit(4*i+2);
        parity3 ^= decoded->getBit(4*i+3);
    }
    parityError = (parity0 ^ errorBlk->getBit(36*16 +32)) 
				| (parity1 ^ errorBlk->getBit(36*16 +33)) 
				| (parity2 ^ errorBlk->getBit(36*16 +34)) 
				| (parity3 ^ errorBlk->getBit(36*16 +35));
	return parityError;
}

void DUO36bx4::CorrectByParity(ECCWord* msg, Block* errorBlk, int chipID){
	bool cor0, cor1, cor2, cor3;
	cor0 = cor1 = cor2 = cor3 = false;
	int cnt=0;
	for(int chip=0; chip<8; chip++){
		if(chip!=chipID){
			if(chip%2==0){
				cor0 ^= msg->getBit(chip*68+64); 		
				cor1 ^= msg->getBit(chip*68+65); 		
				cor2 ^= msg->getBit(chip*68+66); 		
				cor3 ^= msg->getBit(chip*68+67); 		
			}else{
				cor0 ^= msg->getBit(chip*68+0); 		
				cor1 ^= msg->getBit(chip*68+1); 		
				cor2 ^= msg->getBit(chip*68+2); 		
				cor3 ^= msg->getBit(chip*68+3); 		
			}
			cnt++;
//printf("@ chip %i: %i %i %i %i cnt: %i\n", chip, cor0, cor1, cor2, cor3, cnt);
		}
	}
	cor0 ^= errorBlk->getBit(8*68+64);
	cor1 ^= errorBlk->getBit(8*68+65);
	cor2 ^= errorBlk->getBit(8*68+66);
	cor3 ^= errorBlk->getBit(8*68+67);

//printf("final: %i %i %i %i cnt: %i\n", cor0, cor1, cor2, cor3, cnt);

	if(chipID%2==0){	
		msg->setBit(chipID*68+64, cor0);
		msg->setBit(chipID*68+65, cor1);
		msg->setBit(chipID*68+66, cor2);
		msg->setBit(chipID*68+67, cor3);
	}else{
		msg->setBit(chipID*68+ 0, cor0);
		msg->setBit(chipID*68+ 1, cor1);
		msg->setBit(chipID*68+ 2, cor2);
		msg->setBit(chipID*68+ 3, cor3);
	}
		
}
