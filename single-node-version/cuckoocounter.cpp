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
//#define END_FILE_NO 10 //CAIDA
//#define END_FILE_NO 9 //webpage

#define landa_h 8
#define test_cycles 10
#define k 1000
#define c1 1 
#define c2 1
#define c3 1
#define hh 0.0002
#define hc 0.0005
#define epoch 10 
#define BUCKET_NUM (HEAVY_MEM / 8)
#define maxloop 2

struct FIVE_TUPLE { char key[13]; }; //CAIDA
//struct FIVE_TUPLE { char key[8]; }; //webpage
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
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

inline uint8_t finger_print(unsigned int hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash & 255;
}

class hg_node
{
public:
	uint8_t fingerprint[4];
	uint8_t count12;
	uint8_t count3;
	uint16_t count4;
	hg_node()
	{
		for (int i = 0; i < 4; i++)
			fingerprint[i] = 0;
		count12 = 0;
		count3 = 0;
		count4 = 0;
	}
};

vector<hg_node>hg(BUCKET_NUM);

bool cmp1(pair<int, int>p1, pair<int, int>p2)
{
	return p1.first > p2.first;
}

bool cmp2(pair<int, int>p1, pair<int, int>p2)
{
	return p1.second > p2.second;
}

bool overflow(int hash, int j)
{
	if (j == 1) {
		if (hg[hash].count3 < 0xf) {
			char tmp = hg[hash].fingerprint[0];
			hg[hash].fingerprint[0] = hg[hash].fingerprint[2];
			hg[hash].fingerprint[2] = tmp;
			hg[hash].count12 = hg[hash].count12&0xf0 | hg[hash].count3;
			hg[hash].count3 = 0xf;
		}
		else if (hg[hash].count4 < 0xf) {
			char tmp = hg[hash].fingerprint[0];
			hg[hash].fingerprint[0] = hg[hash].fingerprint[3];
			hg[hash].fingerprint[3] = tmp;
			hg[hash].count12 = hg[hash].count12&0xf0 | (uint8_t)hg[hash].count4;
			hg[hash].count4 = 0xf;
		}
		else return false;
	}
	else if (j == 2) {
		if (hg[hash].count3 < 0xf) {
			char tmp = hg[hash].fingerprint[1];
			hg[hash].fingerprint[1] = hg[hash].fingerprint[2];
			hg[hash].fingerprint[2] = tmp;
			hg[hash].count12 = hg[hash].count12&0xf | (hg[hash].count3<<4);
			hg[hash].count3 = 0xf;
		}
		else if (hg[hash].count4 < 0xf) {
			char tmp = hg[hash].fingerprint[1];
			hg[hash].fingerprint[1] = hg[hash].fingerprint[3];
			hg[hash].fingerprint[3] = tmp;
			hg[hash].count12 = hg[hash].count12&0xf | (uint8_t)(hg[hash].count4<<4);
			hg[hash].count4 = 0xf;
		}
		else return false;
	}
	else if (j == 3) {
		if (hg[hash].count4 < 0xff) {
			char tmp = hg[hash].fingerprint[2];
			hg[hash].fingerprint[2] = hg[hash].fingerprint[3];
			hg[hash].fingerprint[3] = tmp;
			hg[hash].count3 = (uint8_t)hg[hash].count4;
			hg[hash].count4 = 0xff;
		}
		else return false;
	}
	return true;
}

bool addone(int hash, int j)
{
	if (j == 1) hg[hash].count12++;
	else if (j == 2) hg[hash].count12 += 0x10;
	else if (j == 3) hg[hash].count3++;
	else if (j == 4) hg[hash].count4++;
	if (((hg[hash].count12&0xf)==0xf)&&(j==1) || ((hg[hash].count12&0xf0)==0xf0)&&(j==2) || (hg[hash].count3==0xff)&&(j==3)) {
		bool res = overflow(hash, j);
		if (!res) return false;
	}
	return true;
}

void kickout(int loop, uint8_t count, int hash1, int hash2, int j)
{
	int rehash = hash2;
	if (hg[rehash].fingerprint[0] == 0 && count < 0xf) {
		hg[rehash].fingerprint[0] = hg[hash1].fingerprint[j-1];
		hg[rehash].count12 = (hg[rehash].count12 & 0xf0) | count;
	}
	else if (hg[rehash].fingerprint[1] == 0 && count < 0xf) {
		hg[rehash].fingerprint[1] = hg[hash1].fingerprint[j-1];
		hg[rehash].count12 = (hg[rehash].count12 & 0xf) | (count << 4);
	}
	else if (hg[rehash].fingerprint[2] == 0 && count < 0xff) {
		hg[rehash].fingerprint[2] = hg[hash1].fingerprint[j-1];
		hg[rehash].count3 = count;
	}
	else if (hg[rehash].fingerprint[3] == 0 && count < 0xffff) {
		hg[rehash].fingerprint[3] = hg[hash1].fingerprint[j-1];
		hg[rehash].count4 = (uint16_t)count;
	}
	else if (loop > 0) {
		loop--;
		if (count < 0xf) {
			uint8_t tmpcount = hg[rehash].count12 & 0xf;
			kickout(loop, tmpcount, rehash, hash1, 1);
			hg[rehash].fingerprint[0] = hg[hash1].fingerprint[j-1];
			hg[rehash].count12 = (hg[rehash].count12&0xf0) | count;					
		}
		else if (count < 0xff) {
			uint8_t tmpcount = hg[rehash].count3;
			kickout(loop, tmpcount, rehash, hash1, 3);
			hg[rehash].fingerprint[2] = hg[hash1].fingerprint[j-1];
			hg[rehash].count3 = count;
		}
		else {
			uint16_t tmpcount = hg[rehash].count4;
			kickout(loop, tmpcount, rehash, hash1, 4);
			hg[rehash].fingerprint[3] = hg[hash1].fingerprint[j-1];
			hg[rehash].count4 = (uint16_t)count;
		}
	}
	else {
		if (count < 0xf) {
			hg[rehash].fingerprint[1] = hg[hash1].fingerprint[j-1];
			if (count > ((hg[rehash].count12&0xf0)>>4))
				hg[rehash].count12 = (hg[rehash].count12&0xf) | (count<<4);
		}
		else if (count < 0xff) {
			hg[rehash].fingerprint[2] = hg[hash1].fingerprint[j-1];
			if (count > hg[rehash].count3)
				hg[rehash].count3 = count;
		}
	}
}

void kickoverflow(int hash1, int hash2, int j)
{
	int rehash = hash2;
	if (j == 1 || j == 2) {
		if (hg[rehash].fingerprint[2] == 0) {
			hg[rehash].fingerprint[2] = hg[hash1].fingerprint[j-1];
			hg[rehash].count3 = 0xf;
		}
		else if (hg[rehash].fingerprint[3] == 0) {
			hg[rehash].fingerprint[3] = hg[hash1].fingerprint[j-1];
			hg[rehash].count4 = 0xf;
		}
		else if (hg[rehash].count3 < 0xf) {
			kickout(maxloop, hg[rehash].count3, rehash, hash1, 3);
			hg[rehash].fingerprint[2] = hg[hash1].fingerprint[j-1];
			hg[rehash].count3 = 0xf;
		}
		else if (hg[rehash].count4 < 0xf) {
			kickout(maxloop, hg[rehash].count4, rehash, hash1, 4);
			hg[rehash].fingerprint[3] = hg[hash1].fingerprint[j-1];
			hg[rehash].count4 = 0xf;
		}
		else return;
		if (j == 1) hg[hash1].count12 &= 0xf0;
		else hg[hash1].count12 &= 0xf;
	}
	else if (j == 3) {
		if (hg[rehash].fingerprint[3] == 0) {
			hg[rehash].fingerprint[3] = hg[hash1].fingerprint[j-1];
			hg[rehash].count4 = 0xff;
		}
		else if (hg[rehash].count4 < 0xff) {
			kickout(maxloop, hg[rehash].count4, rehash, hash1, 4);
			hg[rehash].fingerprint[3] = hg[hash1].fingerprint[j-1];
			hg[rehash].count4 = 0xff;
		}
		else return;
		hg[hash1].count3 = 0;
	}
	hg[hash1].fingerprint[j-1] = 0;
}

void Insert(int hash1, int hash2, uint8_t fp)
{
	int hash[2] = {hash1, hash2};
	int ii, jj, flag = 0;
	for (int i = 0; i < 2; i++) {
		for(int j = 1; j <= 4; j++) {
			if (hg[hash[i]].fingerprint[j-1] == fp) {
				bool res = addone(hash[i], j);
				if (!res) kickoverflow(hash[i], hash[1-i], j);
				return;
			}
			else if (!flag && hg[hash[i]].fingerprint[j-1] == 0) {
				ii = i;
				jj = j;
				flag = 1;
			}
		}
	}
	if (flag) {
		hg[hash[ii]].fingerprint[jj-1] = fp;
		addone(hash[ii], jj);
		return;
	}
	int i = rand() % 2;
	uint8_t count = hg[hash[i]].count12 & 0xf;
	kickout(maxloop, count, hash[i], hash[1-i], 1);
	hg[hash[i]].fingerprint[0] = fp;
	hg[hash[i]].count12 = (hg[hash[i]].count12&0xf0) | 0x1;	
}

int query(int hash, int j)
{
	if (j == 1) return (hg[hash].count12&0xf);
	else if (j == 2) return ((hg[hash].count12&0xf0) >> 4);
	else if (j == 3) return hg[hash].count3;
	else if (j == 4) return hg[hash].count4;
	return 0;
}

int Query(int hash1, int hash2, int fp)
{
	int hash[2] = {hash1, hash2};
	for (int i = 0; i < 2; i++) {
		for (int j = 1; j <= 4; j++) {
			if (hg[hash[i]].fingerprint[j-1] == fp)
				return query(hash[i], j);
		}
	}
	double min1=min(hg[hash[0]].count12&0xf, hg[hash[1]].count12&0xf);

	double min2=(min(hg[hash[0]].count12&0xf0, hg[hash[1]].count12&0xf0))>>4;

	return min(min1, min2);
	//int i = rand() % 2;
	//return (bucket[i][hash[i]].count12&0xf) / (1+(bucket[i][hash[i]].count12&0xf)/k);
}

void ReadInTraces(const char* trace_prefix)
{
	for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		char datafileName[100];
		sprintf(datafileName, "demo.dat"); //demo
		//sprintf(datafileName, "%s%d.dat", trace_prefix, datafileCnt - 1); //CAIDA
		//sprintf(datafileName, "%swebdocs_form0%d.dat", trace_prefix, datafileCnt - 1); //webpage
		FILE* fin = fopen(datafileName, "rb");
		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt - 1].clear();
		while (fread(&tmp_five_tuple, 1, 13, fin) == 13) { //CAIDA
		//while (fread(&tmp_five_tuple, 1, 8, fin) == 8) { //webpage
			traces[datafileCnt - 1].push_back(tmp_five_tuple);
		}
		fclose(fin);
		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt - 1].size());
	}
	printf("\n");
}

int main()
{
	ReadInTraces("./");
	//ReadInTraces("./data/CAIDA/");
	//ReadInTraces("./data/webpage/");
	//ofstream out("./dhs.txt");
	//ofstream state("./state.txt");
	//out<<"memory: "<<HEAVY_MEM<<endl;
	cout<<"memory: "<<HEAVY_MEM<<endl;
	vector<double>gb_heavy_changer(10, 0);
	vector<int>hc_cnt(10,0);
	vector<double>gb_heavy_hitter(10, 0);
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
		for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt) {
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
			//printf("packet count:%d\n", packet_cnt);
	
			char** keys = new char * [(int)traces[datafileCnt - 1].size()];
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); i++) {
				keys[i] = new char[13]; //CAIDA
				memcpy(keys[i], traces[datafileCnt - 1][i].key, 13); //CAIDA
				//keys[i] = new char[8]; //webpage
				//memcpy(keys[i], traces[datafileCnt - 1][i].key, 8); //webpage
			}
	
			double th = 0;
			clock_gettime(CLOCK_MONOTONIC, &time1);
			std::fill(hg.begin(), hg.end(), hg_node()); 
			for (int i = 0; i < packet_cnt; i++) {	
				int hash = BKDRHash(keys[i], 13); //CAIDA
				//int hash = BKDRHash(keys[i], 8); //webpage
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				Insert(hash1, hash2, fp);
			}
			clock_gettime(CLOCK_MONOTONIC, &time2);
			resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
			th = (double)1000.0 * packet_cnt / resns;
			cout<<"throughput: "<<th<<"mpps"<<endl;
			gb_throughput += th;
			
			
			std::fill(hg.begin(), hg.end(), hg_node());
			for (int i = 0; i < packet_cnt; i++) {
				int hash = BKDRHash(keys[i], 13); //CAIDA
				//int hash = BKDRHash(keys[i], 8); //webpage
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				gb_cnt[hash] += 1;
				lc_cnt[hash] += 1;
				if (hit_cnt.count(hash) == 0) hit_cnt[hash] = Query(hash1, hash2, fp);
				Insert(hash1, hash2, fp);
				//cout<<i<<"th insertion"<<endl;
				if(i && i % window == 0) {
					//heavy hitter
					if(1) {
						double th = window * hh;
						int tp = 0, fp =0, tn = 0, fn = 0;
						for (auto it:gb_cnt) {
							bool f1 = 0, f2 = 0;
							if (it.second >= th) f1 = 1;
							int hash = it.first;
							uint8_t fp = finger_print(hash);
							int hash1 = hash % BUCKET_NUM;
							int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
							int efreq = Query(hash1, hash2, fp);
							if (efreq >= th) f2 = 1;
							if (f1 && f2) tp++;
							else if (f1 && !f2) fn++;
							else if (!f1 && f2) fp++;
							else if (!f1 && !f2) tn++;
						}
						double recall = (double)tp / (tp + fn);
						double precision = (double)tp / (tp + fp);
						double fscore = 2 * (recall * precision) / (recall + precision);
						//cout<<recall<<" "<<precision<<endl;
						//out<<i/window <<"th heavy hitter FSOCRE:"<<fscore<<endl;
						gb_heavy_hitter[i/window-1] += fscore;
						hh_cnt[i/window-1] += 1;
					}
					//heavy changer
					if(1) {
						double th = window * hc;
						int tp = 0, fp =0, tn = 0, fn = 0;
						for (auto it:lc_cnt) {
							bool f1 = 0, f2 = 0;
							if (it.second >= th) f1 = 1;
							int hash = it.first;
							uint8_t fp = finger_print(hash);
							int hash1 = hash % BUCKET_NUM;
							int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
							int efreq = Query(hash1, hash2, fp);
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
						//cout<<recall<<" "<<precision<<endl;
						//out<<i/window <<"th heavy hitter FSOCRE:"<<fscore<<endl;
						gb_heavy_changer[i/window-1] += fscore;
						hc_cnt[i/window-1] += 1;
					}		
					lc_cnt = unordered_map<int,int>();
					hit_cnt = unordered_map<int,int>();
				}
			}
			
			vector<pair<int, int>>topk;
			for (auto it:gb_cnt) {
				topk.push_back(make_pair(it.first, it.second));
			}
			sort(topk.begin(), topk.end(), cmp2);
			//AAE
			double AAE = 0;
			for (int i = 0; i < topk.size(); i++) {
				int hash = topk[i].first;
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				int efq = Query(hash1, hash2, fp);
				AAE += (double)abs(topk[i].second - efq);
			}
			AAE /= gb_cnt.size();
			cout<<"AAE: "<<AAE<<endl;
			gb_AAE += AAE;
			//ARE				
			double ARE = 0;
			for(int i = 0; i < topk.size(); i++) {
				int hash = topk[i].first;
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				int efq = Query(hash1, hash2, fp);
				//printf("real freq:%d, estimated freq:%d\n", topk[i].second, efq);
				ARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			ARE /= gb_cnt.size();
			cout<<"ARE: "<<ARE<<endl;
			gb_ARE += ARE;
			//Topk ARE
			double kARE = 0;
			for(int i = 0; i < k; i++) {
				int hash = topk[i].first;
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				int efq = Query(hash1, hash2, fp);
				//printf("real freq:%d, estimated freq:%d\n", topk[i].second, efq);
				kARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			kARE /= k;
			//out<<"Topk ARE: "<<kARE<<endl;
			cout<<"Topk ARE: "<<kARE<<endl;
			gb_kARE += kARE;
			//WMRE
			int max_freq = 0;
			for (auto it:gb_cnt) {
				freq[it.second] += 1;
				int hash = it.first;
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				int efreq = Query(hash1, hash2, fp);
				freq_e[efreq] += 1;
				max_freq = max(max_freq, it.second);
				max_freq = max(max_freq, efreq);
			}
			double wmre = 0, wmd = 0;
			for (int i = 1; i <= max_freq; i++) {
				wmre += (double)abs(freq[i] - freq_e[i]);
				wmd += ((double)freq[i] + freq_e[i]) / 2;
			}
			//out<<"WMRE: "<<wmre/wmd<<endl;
			cout<<"WMRE: "<<wmre/wmd<<endl;
			gb_WMRE += wmre/wmd;
			//entropy
			int flow_num = gb_cnt.size();
			cout<<"flow_num: "<<flow_num<<endl;
			double e = 0, ee = 0;
			for (int i = 1; i <= max_freq; i++) {
				e += freq[i]?-1*((double)i*freq[i]/flow_num)*log2((double)freq[i]/flow_num):0;
				ee += freq_e[i]?-1*((double)i*freq_e[i]/flow_num)*log2((double)freq_e[i]/flow_num):0;
			}
			//out<<"entropy ARE: "<<fabs(e-ee)/e<<endl;
			cout<<"entropy ARE: "<<fabs(e-ee)/e<<endl;
			//out<<"real entropy: "<<e<<endl; 
			gb_entropy += fabs(e - ee) / e;
			//FSCORE
			unordered_map<int, bool>ef;
			double threshold = packet_cnt * hh;
			int tp = 0, fp =0, tn = 0, fn = 0;
			for (auto it:gb_cnt) {
				bool f1 = 0, f2 = 0;
				if (it.second >= threshold) f1 = 1;
				int hash = it.first;
				uint8_t fp = finger_print(hash);
				int hash1 = hash % BUCKET_NUM;
				int hash2 = (hash1 ^ BKDRHash((char *)&fp)) % BUCKET_NUM;
				int efreq = Query(hash1, hash2, fp);
				if (efreq >= threshold) f2 = 1;
				if (f1 && f2) tp++;
				else if (f1 && !f2) fn++;
				else if (!f1 && f2) fp++;
				else if (!f1 && !f2) tn++;
			}
			double recall = (double)tp / (tp + fn);
			double precision = (double)tp / (tp + fp);
			double fscore = 2 * (recall * precision) / (recall + precision);
			//cout<<recall<<" "<<precision<<endl;
			//out<<"Total FSOCRE: "<<fscore<<endl;
			cout<<"Total FSOCRE: "<<fscore<<endl;
			gb_fscore += fscore;
	
			/* free memory */
			for (int i = 0; i < packet_cnt; ++i)
				delete[] keys[i];
			delete[] keys;
			cout<<"finish free\n"<<endl;
		}
	}
	cout<<"heavy hitter FSCORE: "<<endl;
	for(int i = 0; i < 10; i++) {
		if (hh_cnt[i] > 0) cout<<gb_heavy_hitter[i] / hh_cnt[i]<<endl;
	}
	cout<<"heavy changer FSCORE: "<<endl;
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
