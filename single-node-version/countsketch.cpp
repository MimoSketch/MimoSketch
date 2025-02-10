#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <stdint.h>
#include <random>
#include <string>
#include <memory>
#include <iostream>
#include <cmath>
#include <math.h>
#include<time.h>
#include<unordered_map>
#include<algorithm>
#include<fstream>
#include <bitset>

using namespace std;

#define START_FILE_NO 1
#define END_FILE_NO 1 //demo

#define landa_d 16
#define b 1.08 
#define test_cycles 1
#define k 1000
#define hh 0.0002
#define hc 0.0005
#define epoch 10 
#define HASH_NUM 3
#define BUCKET_NUM (HEAVY_MEM / (4 * HASH_NUM))
#define INT_MAX ((int)(~0U>>1))

struct FIVE_TUPLE { char key[13]; }; //demo
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static const int COUNT[2] = {1, -1};
static const int factor = 1;
static int HEAVY_MEM = 1000 * 1024;

inline unsigned int BKDRHash(char* str, int len = 1)
{
	unsigned int seed = 131;
	unsigned int hash = 0;
	for (int i = 0; i < len; i++) {
		hash = hash * seed + (*str++);
	}
	return (hash & 0x7FFFFFFF);
}

inline unsigned int finger_print(unsigned int hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash & 0x7FFFFFFF;
}

class hg_node
{
public:
	vector<int>counters; //32*BUCKET_NUM
	hg_node()
	{
		counters = vector<int>(BUCKET_NUM, 0);
	}
};

vector<hg_node>hg(HASH_NUM);

void Insert(unsigned int item)
{
	for (int i = 0; i < HASH_NUM; i++) {
		unsigned int choice = (item>>i) & 1;
		int position = finger_print(item*(i+1)) % BUCKET_NUM;
		hg[i].counters[position] += COUNT[choice];
	}
}

int Query(unsigned int item)
{
	int result[HASH_NUM];
	for (int i = 0; i < HASH_NUM; i++) {
		unsigned int choice = (item>>i) & 1;
		int position = finger_print(item*(i+1)) % BUCKET_NUM;
		result[i] = hg[i].counters[position] * COUNT[choice];
	}
	sort(result, result + HASH_NUM);
	return max(result[HASH_NUM/2], 0);
	//result /= HASH_NUM;
	//return max(result, 0);
}

void ReadInTraces(const char* trace_prefix)
{
	for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		char datafileName[100];
		sprintf(datafileName, "demo.dat"); //demo
		FILE* fin = fopen(datafileName, "rb");
		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt - 1].clear();
		while (fread(&tmp_five_tuple, 1, 13, fin) == 13) { //demo
			traces[datafileCnt - 1].push_back(tmp_five_tuple);
		}
		fclose(fin);
		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt - 1].size());
	}
	printf("\n");
}

bool cmp1(pair<int, int>p1, pair<int, int>p2)
{
	return p1.first > p2.first;
}

bool cmp2(pair<int, int>p1, pair<int, int>p2)
{
	return p1.second > p2.second;
}

int main()
{
	ReadInTraces("./");
	cout<<"memory: "<<HEAVY_MEM<<endl;
	vector<double> gb_heavy_changer(10, 0);
	vector<int>hc_cnt(10,0);
	vector<double> gb_heavy_hitter(10, 0);
	vector<int>hh_cnt(10,0);
	double gb_AAE = 0;
	double gb_ARE = 0;
	double gb_kARE = 0;
	double gb_WMRE = 0;
	double gb_entropy = 0;
	double gb_fscore = 0;
	double gb_throughput = 0;
 	
 	for (int i = 0; i < test_cycles; i++) {	
		srand((unsigned)time(NULL));
		for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; datafileCnt++) {
			unordered_map<int,int>gb_cnt;
			unordered_map<int,int>lc_cnt;
			unordered_map<int,int>pre_cnt;
			unordered_map<int,int>hit_cnt;
			unordered_map<int,int>ph_cnt;
			unordered_map<int,int>freq;
			unordered_map<int,int>freq_e;
			timespec time1, time2;
			long long resns;
			int packet_cnt = (int)traces[datafileCnt - 1].size();
			int window = packet_cnt / epoch;
	
			char** keys = new char * [(int)traces[datafileCnt - 1].size()];
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); i++) {
				keys[i] = new char[13]; //demo
				memcpy(keys[i], traces[datafileCnt - 1][i].key, 13); //demo
			}
	
			double th = 0;
			clock_gettime(CLOCK_MONOTONIC, &time1); 
			std::fill(hg.begin(), hg.end(), hg_node()); 
			for (int i = 0; i < packet_cnt; i++) {
				int hash = BKDRHash(keys[i], 13); //demo
				Insert(hash);
			}
			clock_gettime(CLOCK_MONOTONIC, &time2);
			resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
			th = (double)1000.0 * packet_cnt / resns;
			cout<<"throughput: "<<th<<"mpps"<<endl;
			gb_throughput += th;
			
			std::fill(hg.begin(), hg.end(), hg_node()); 
			for (int i = 0; i < packet_cnt; i++) {
				int hash = BKDRHash(keys[i], 13); //demo
				gb_cnt[hash] += 1;
				lc_cnt[hash] += 1;
				if(hit_cnt.count(hash) == 0) hit_cnt[hash] = Query(hash);
				Insert(hash);
				if(i && i % window == 0) {
					//heavy hitter
					if(1) {
						double th = window * hh;
						int tp = 0, fp =0, tn = 0, fn = 0;
						for (auto it:gb_cnt) {
							bool f1 = 0, f2 = 0;
							if(it.second >= th) f1 = 1;
							int hash = it.first;
							int efreq = Query(hash);
							if(efreq >= th) f2 = 1;
							if(f1 && f2) tp++;
							else if(f1 && !f2) fn++;
							else if(!f1 && f2) fp++;
							else if(!f1 && !f2) tn++;
						}
						double recall = (double)tp / (tp + fn);
						double precision = (double)tp / (tp + fp);
						double fscore = 2 * (recall * precision) / (recall + precision);
						gb_heavy_hitter[i/window-1] += fscore;
						hh_cnt[i/window-1] += 1;
					}
					//heavy change
					if(1) {
						double th = window * hc;
						int tp = 0, fp =0, tn = 0, fn = 0;
						for (auto it:lc_cnt) {
							bool f1 = 0, f2 = 0;
							if (it.second >= th) f1 = 1;
							int hash = it.first;
							int efreq = Query(hash) - hit_cnt[hash];
							ph_cnt[hash] = efreq;
							if (efreq >= th) f2 = 1;
							if (f1 && f2) tp++;
							else if (f1 && !f2) fn++;
							else if (!f1 && f2) fp++;
							else if (!f1 && !f2) tn++;
						}
						double recall = (double)tp / (tp + fn);
						double precision = (double)tp / (tp + fp);
						double fscore = 2 * (recall * precision) / (recall + precision);
						gb_heavy_changer[i/window-1] += fscore;
						hc_cnt[i/window-1] += 1;
					}
					lc_cnt = unordered_map<int,int>();
					hit_cnt = unordered_map<int,int>();
				}
			}
			
			vector<pair<int,int>>topk;
			for(auto it:gb_cnt) {
				topk.push_back(make_pair(it.first, it.second));
			}
			sort(topk.begin(), topk.end(), cmp2);
			//AAE
			double AAE = 0;
			for(int i =0; i<topk.size(); i++) {
				int hash = topk[i].first;
				int efq = Query(hash);
				AAE += (double)abs(topk[i].second - efq);
			}
			AAE /= topk.size();
			cout<<"AAE: "<<AAE<<endl;
			gb_AAE += AAE;
			//ARE
			double ARE = 0;
			for(int i =0; i<topk.size(); i++) {
				int hash = topk[i].first;
				int efq = Query(hash);
				ARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			ARE /= topk.size();
			cout<<"ARE: "<<ARE<<endl;
			gb_ARE += ARE;
			//Topk ARE
			double kARE = 0;
			for(int i =0; i<k; i++) {
				int hash = topk[i].first;
				int efq = Query(hash);
				kARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			kARE /= k;
			cout<<"Topk ARE: "<<kARE<<endl;
			gb_kARE += kARE;
			//WMRE
			int max_freq = 0;
			for(auto it:gb_cnt) {
				freq[it.second] += 1;
				int hash = it.first;
				int efreq = Query(hash);
				freq_e[efreq] += 1;
				max_freq = max(max_freq, it.second);
				max_freq = max(max_freq, efreq);
			}
			double wmre = 0, wmd = 0;
			for(int i = 1; i <= max_freq; i++) {
				wmre += (double)abs(freq[i] - freq_e[i]);
				wmd += ((double)freq[i] + freq_e[i]) / 2;
			}
			cout<<"WMRE:"<<wmre/wmd<<endl;
			gb_WMRE += wmre/wmd;
			//entropy
			int flow_num = gb_cnt.size();
			double e = 0, ee = 0;
			for(int i = 1; i<=max_freq; i++) {
				e += freq[i]?-1*((double)i*freq[i]/flow_num)*log2((double)freq[i]/flow_num):0;
				ee += freq_e[i]?-1*((double)i*freq_e[i]/flow_num)*log2((double)freq_e[i]/flow_num):0;
			}
			cout<<"entropy ARE: "<<fabs(e-ee)/e<<endl;
			gb_entropy += fabs(e-ee)/e;
			//FSCORE
			unordered_map<int,bool>ef;
			double threshold = packet_cnt * hh;
			int tp = 0, fp =0, tn = 0, fn = 0;
			for(auto it:gb_cnt) {
				bool f1 = 0, f2 = 0;
				if (it.second >= threshold) f1 = 1;
				int hash = it.first;
				int efreq = Query(hash);
				if (efreq >= threshold) f2 = 1;
				if (f1 && f2) tp++;
				else if (f1 && !f2) fn++;
				else if (!f1 && f2) fp++;
				else if (!f1 && !f2) tn++;
			}
			double recall = (double)tp / (tp + fn);
			double precision = (double)tp / (tp + fp);
			double fscore = 2 * (recall * precision) / (recall + precision);
			cout<<"Total FSCORE: "<<fscore<<endl;
			gb_fscore += fscore;
			
			/* free memory */
			for (int i = 0; i < packet_cnt; i++)
				delete[] keys[i];
			delete[] keys;
			cout<<"finish free\n"<<endl;
		}
	}
	cout<<"heavy hitter FSCORE: "<<endl;
	for(int i = 0; i < 10; i++) {
		if (hh_cnt[i] > 0) cout<<gb_heavy_hitter[i] / hh_cnt[i]<<endl;
	}
	cout<<"heavy change FSCORE: "<<endl;
	for(int i = 0; i < 10; i++) {
		if (hc_cnt[i] > 0) cout<<gb_heavy_changer[i] / hc_cnt[i]<<endl;
	}
	cout<<"AAE: "<<gb_AAE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"ARE: "<<gb_ARE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"Topk ARE: "<<gb_kARE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"WMRE: "<<gb_WMRE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"entropy ARE: "<<gb_entropy / (END_FILE_NO * test_cycles)<<endl;
	cout<<"Total FSCORE: "<<gb_fscore / (END_FILE_NO * test_cycles)<<endl;
	cout<<"throughput: "<<gb_throughput / (END_FILE_NO * test_cycles)<<endl;
	cout<<endl;
}
