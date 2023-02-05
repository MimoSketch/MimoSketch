#pragma once

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <bitset>
#include "countsketch.h"

#define BUCKET_NUM_WS (HEAVY_MEM / 134)
#define landa_d 16
#define TOT_MEM_IN_BYTES (600 * 1024)
#define INT_MAX ((int)(~0U>>1))

static const int factor = 1;

class hg_node_ws
{
public:
	std::vector<unsigned int>items; //32*16
	std::vector<int>counters; //32*16
	std::bitset<landa_d>real; //16
	int incast; //32
	
	hg_node_ws()
	{
		items = std::vector<unsigned int>(landa_d, 0);
		counters = std::vector<int>(landa_d, 0);
		incast = 0;
		real.reset();
	}
	void Insert(int hash, unsigned int item)
	{
		unsigned int choice = hash;
        int min_num = INT_MAX;
        unsigned int min_pos = -1;
        
        for (unsigned int i = 0; i < landa_d; i++) {
            if (counters[i] == 0) {
                items[i] = item;
                counters[i] = 1;
                real[i] = 1;
                return;
            }
            else if (items[i] == item) {
                if (real[i])            
                    counters[i]++;
                else {
                    counters[i]++;
                    incast += COUNT[choice];
                }
                return;
            }

            if (counters[i] < min_num) {
                min_num = counters[i];
                min_pos = i;
            }
        }

        /*
        if(incast * COUNT[choice] >= min_num){
            //count_type pre_incast = incast;
            if(real[min_pos]){
                uint32_t min_choice = hash_(items[min_pos], 17) & 1;
                incast += COUNT[min_choice] * counters[min_pos];
            }
            items[min_pos] = item;
            counters[min_pos] += 1;
            real[min_pos] = 0;
        }
        incast += COUNT[choice];
        */

        if (incast * COUNT[choice] >= int(min_num * factor)) {
            //count_type pre_incast = incast;
            if (real[min_pos]) {
                unsigned int min_choice = items[min_pos] & 1;
                incast += COUNT[min_choice] * counters[min_pos];
            }
            items[min_pos] = item;
            counters[min_pos] += 1;
            real[min_pos] = 0;
        }
        incast += COUNT[choice];
	}
	int Query(int hash, unsigned int item)
	{
		unsigned int choice = hash;

        for (unsigned int i = 0; i < landa_d; i++) {
            if (items[i] == item) { //if (items[i] == item && real[i]) {
                return counters[i];
            }
        }

        return 0;
	    //return max(incast * COUNT[choice], 0);
	}
};
