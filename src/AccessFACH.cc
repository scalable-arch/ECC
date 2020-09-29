#include "AccessFACH.hh"
#include <bitset>

static FaultCache* fach;

static int cache_sets = DEFAULT_CACHE_SETS;
static int cache_way = DEFAULT_N_WAY;

static unsigned row_mask, col_mask, bank_mask, rank_mask;
static unsigned row_mask_offset, bank_mask_offset, rank_mask_offset;
std::vector<int> set_bit_pos;  // contatining which location will change
 std::bitset<64> bt;

void showCacheStatus(){

    std::cout<<"-----------------------------------------------------------------"<<std::endl;
    std::cout<<"------------------(type)(row)(bank)(col)(counter)----(type)(row)(bank)(col)(counter)" <<std::endl;

    for(int i=0; i<cache_sets; i++){

        std::cout<<i<<"th entry: ";

        for(int k=0; k<fach[i].getNumEntry(); k++){
            int ct = fach[i].entryVector.at(k).counter;
            TAG row = fach[i].entryVector.at(k).row;
            TAG bank = fach[i].entryVector.at(k).bank;
            TAG col = fach[i].entryVector.at(k).col;
            char type = fach[i].entryVector.at(k).type;

            std::cout<<k <<"th way: " << type <<"   ";
            printf("%d  %d   %d   %d  ", row, bank, col, ct);
        }
        std::cout<<std::endl;
    }
    std::cout<<"-----------------------------------------------------------------"<<std::endl;
}


double multiBitFault(int nun_fault, int num_set){
    int numHit = 0;
    std::uniform_int_distribution<unsigned long long> randDist = std::uniform_int_distribution<unsigned long long>(0, set_bit_pos.size()-1);

    // number of generated single faults = 10.
    for(int m=0; m <nun_fault; m++){
        // reset
        for(int i=0; i<set_bit_pos.size(); i++){
            bt[set_bit_pos[i]] = 0;
        }

        // number of bits to be set to 1 =  2  -- it is considered proper to be replaced by numDQ
        for(int k=0; k < num_set; k++){
            int randValue = randDist(randomGenerator);
            bt[set_bit_pos[randValue]] = 1;
        }

        ADDR aa = bt.to_ullong();
        //std::cout<<"new_fault addr: " << std::bitset<64>(aa)<<std::endl;
        numHit += performAccess(aa);
    }
   
    return (double)numHit/20;
}


double genSingleFault(Fault* _fault){

    ADDR addr = _fault->addr;
    ADDR mask = _fault->mask;
    
    int numDQ = _fault->numDQ;
    int affectedblkCount = _fault->affectedBlkCount;

    int numHit = 0, iter =0;   
    for(unsigned i=1; iter<33; i<<=1, iter++){
        if(mask & i)
            set_bit_pos.push_back(iter);
    }

    bt = std::bitset<64>(addr);

    //std::cout<<"set_bit_pos size: " <<set_bit_pos.size()<<std::endl;
    std::default_random_engine randomGenerator;
    
    std::string name = _fault->name;

    if(name=="S-bit"){
        return performAccess(addr);
    } else if(name=="S-word") {
    
        for(int i=0; i<numDQ; i++)
            numHit += performAccess(addr+i);
        return (double)numHit/numDQ;

    } else if(name=="S-col") {
        return multiBitFault(5, 1);
    } else if(name=="S-row") {
        return multiBitFault(5, 2);
    } else if(name=="S-bank") {
        return multiBitFault(20, 3);
    } else if(name=="M-bank") {
        return multiBitFault(40, 4);
    } else if(name=="M-rank") {
        return multiBitFault(100, 5);
    } else {
        assert(0);
        return NULL;
    }
}

int performAccess(ADDR addr){
    
    TAG rank = (addr & rank_mask) >> rank_mask_offset;
    TAG bank = (addr & bank_mask) >> bank_mask_offset;
    //printf("addr: %llx  \n", addr);
    TAG index_row = (addr & row_mask) >> (row_mask_offset + INDEX_ROW_OFFSET);
    TAG raw_row = (addr & row_mask) >> row_mask_offset;

    TAG index_col = (addr & col_mask) >> INDEX_COL_OFFSET;
    TAG raw_col = (addr & col_mask);
    
    int index = index_row^index_col;

    //std::cout << "new_fault's raw_row: "<< raw_row << " raw_col: " << raw_col<<"index:  " <<std::endl; 

    // Start coalescing 
    fach[index].coalesEntry(rank, bank, raw_row, raw_col);

    // Hit in Cache;
    if(fach[index].isHit(rank, raw_row, bank, raw_col)){
       //std::cout << "Cache Hit!! returning 1"<<std::endl<<std::endl; 
       return 1;
      // Cache Miss   
    } else { 
    
        // there is empty space in cache
        if(fach[index].getNumEntry() < cache_way){
            fach[index].insertNewEntry(rank, raw_row, bank, raw_col);
            //std::cout << "New Line inserted returning 0 "<<std::endl<<std::endl; 
        }else{
            fach[index].evictEntry();
            fach[index].insertNewEntry(rank, raw_row, bank, raw_col);
            //std::cout << "Replaced!! returning 0 "<<std::endl<<std::endl; 
        }
    }
    
    for(int i=0; i<cache_sets; i++)
        fach[i].decreCounter();
    return 0;
}

void setCacheParam(int param, int value){

    switch (param)
    {
    case CACHE_PARAM_SETS:
        cache_sets = value;
        break;
    
    case CACHE_PARAM_WAY:
        cache_way = value;
        break;
    
    default:
        printf("error set_cache_param: wrong parameter value \n");
        exit(-1);
    }
};


void initCache(){
    fach = new FaultCache[cache_sets];

    for(int i=0; i<cache_sets; i++)
        fach[i].initialize(cache_way);
    
    rank_mask = 0xffffffff & 0x80000000;
    rank_mask_offset = 31;

    row_mask = 0xffffffff & 0x7fff8000;
    row_mask_offset = 15;
    
    bank_mask = 0xffffffff & 0x00007800;
    bank_mask_offset = 11;

    col_mask = 0xffffffff & 0x000007ff;

}

