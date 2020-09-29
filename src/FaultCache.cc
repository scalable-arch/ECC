#include "FaultCache.hh"

FaultCache::FaultCache(){}

void FaultCache::decreCounter(){ 
	for (int i = 0; i < getNumEntry(); i++) {
		if (entryVector.at(i).counter-10 < 0){
			entryVector.at(i).counter = 0;
		}else{
			entryVector.at(i).counter -= 10;
		}
	}
} 

void FaultCache::insertNewEntry(TAG _rank, TAG _row, TAG _bank, TAG _col) {
	assert(getNumEntry() < num_way);

	EachEntry ee;

	ee.rank = _rank;
	ee.row = _row;
	ee.bank = _bank;
	ee.col = _col >> BLOCK_OFFSET;
	ee.cumul_cols.push_back(_col);
	ee.type = 's';
	ee.counter = 500; // default value of counter

	entryVector.push_back(ee);
}

// Evict entry having the smallest counter value.
void FaultCache::evictEntry() {

	assert(getNumEntry() > 0);

	int min = RAND_MAX;

	std::vector<EachEntry>::iterator it2;
	std::vector<EachEntry>::iterator it1 = entryVector.begin();
	for (; it1 != entryVector.end(); ++it1) {
		if (it1->counter < min) {
			min = it1->counter;
			it2 = it1;
		}
			
	}

	entryVector.erase(it2);
}

bool FaultCache::isHit(TAG _rank, TAG _row, TAG _bank, TAG _col) {

	// if ther is no entry, definitely miss!!
	if (getNumEntry() == 0)
		return false;
	

	std::vector<EachEntry>::iterator it1 = entryVector.begin();
	for (; it1 != entryVector.end(); ++it1) {

		switch (it1->type)
		{
		case 's':
		case 'r':{
			TAG block_col = it1->col << BLOCK_OFFSET;

			// row-block hit
			if (it1->rank==_rank && it1->bank==_bank && it1->row==_row && _col-block_col<64){
				it1->counter += 500;

				if(it1->type =='s')
					it1->cumul_cols.push_back(_col);
				return true;
			}
			break;
		}
		case 'c':
			TAG block_row = it1->row << BLOCK_OFFSET;

			// col-block hit
			if (it1->rank==_rank && it1->bank==_bank && it1->col==_col && _row-block_row<64){
				it1->counter += 500;
				return true;
			}			
		}
	}

	return false;

}


void FaultCache::coalesEntry(TAG _rank, TAG _bank, TAG _raw_row, TAG _raw_col){

	std::vector<EachEntry>::iterator it1 = entryVector.begin();
	for (; it1 != entryVector.end(); ++it1) {

		// it is subject to coalescing checking
		if (it1->rank==_rank && it1->bank==_bank && it1->type=='s'){

			TAG block_row = it1->row;
			TAG block_col = it1->col << BLOCK_OFFSET;
			//std::cout <<"checking...row-direction.. " <<"block_row: "<< block_row << " block_col: " << block_col<<std::endl; 
			// coalesced in direction of row.
			if(_raw_col - block_col < 64 && _raw_row==block_row) {
				it1->type = 'r';
				//std::cout <<"coalesced in direction of row, block-row "<< block_row << std::endl; 
			}else{
				std::vector<int>::iterator it11= it1->cumul_cols.begin();
				for(; it11 != it1->cumul_cols.end(); ++it11){
					//std::cout <<"checking...col-direction.. "<< "block_row: "<< block_row << " block_col: " << *it11<<std::endl; 
					// coalesced in direction of col.
					if(*it11==_raw_col && _raw_row - block_row <64){
						it1->row >>= BLOCK_OFFSET;
						it1->col = _raw_col;
						it1->type = 'c';
						//std::cout <<"coalesced in direction of row, block-col "<< _raw_col << std::endl; 
					}
				}
			}
		}
	}	
}


