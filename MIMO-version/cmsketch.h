#pragma once

#include <iostream>

#define HASH_NUM 3
#define BUCKET_NUM_CM (HEAVY_MEM / 4)

class hg_node_cm
{
public:
	int counter; //32*BUCKET_NUM
	
    hg_node_cm()
	{
        counter = 0;
	}

	void Insert()
	{
        counter += 1;
	}
	int Query()
	{
        return counter;
	}
};
