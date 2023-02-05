#pragma once

#include <iostream>

#define HASH_NUM 3
#define BUCKET_NUM_CS (HEAVY_MEM / 4)

static const int COUNT[2] = {1, -1};

class hg_node_cs
{
public:
	int counter; //32*BUCKET_NUM
	
    hg_node_cs()
	{
		counter = 0;
	}

	void Insert(int hash)
	{
		int choice = hash;
        counter += COUNT[choice];
	}
	int Query(int hash)
	{
		int choice = hash;
        return counter * COUNT[choice];
	}
};
