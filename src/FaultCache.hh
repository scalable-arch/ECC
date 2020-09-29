#pragma once
#include "ParamCofig.hh"
#include <iostream>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <iomanip>



class EachEntry{

public:
	int counter;
	TAG rank, row, bank, col;
	char type;
	ADDR tag;
	
	std::vector<int> cumul_cols;
};

class FaultCache {



public:
	FaultCache();
	
	void initialize(int _num_way) { num_way = _num_way; }

	int getNumEntry() { return entryVector.size(); }

	void decreCounter();

	void insertNewEntry(TAG _rank, TAG _row, TAG _bank, TAG _col);

	void evictEntry();

	bool isHit(TAG _rank, TAG _row, TAG _bank, TAG _col);

	void coalesEntry(TAG _rank, TAG _bank, TAG _raw_row, TAG _raw_col);

public:
	int num_way;

	std::vector<EachEntry> entryVector;
};


