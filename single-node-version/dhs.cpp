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

using namespace std;

#define START_FILE_NO 1
#define END_FILE_NO 1 //demo
//#define END_FILE_NO 10 //CAIDA
//#define END_FILE_NO 9 //webpage

#define landa_h 16
#define b 1.08 
#define test_cycles 10
#define k 1000
#define c1 1 
#define c2 1
#define c3 1
#define hh 0.0002
#define hc 0.0005
#define epoch 10 
#define BUCKET_NUM (HEAVY_MEM / 16)

struct FIVE_TUPLE { char key[13]; }; //CAIDA
//struct FIVE_TUPLE { char key[8]; }; //webpage
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static int HEAVY_MEM = 1000 * 1024;

inline unsigned int BKDRHash(char* str, int len = 13) //CAIDA
//inline unsigned int BKDRHash(char* str, int len = 8) //webpage
{
	unsigned int seed = 131;
	unsigned int hash = 0;
	for (int i = 0; i < len; i++) {
		hash = hash * seed + (*str++);
	}
	return (hash & 0x7FFFFFFF);
}

inline unsigned short finger_print(unsigned int hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash & 65535;
}

class hg_node
{
public:
	vector<unsigned char>heavy;
	unsigned int usage;//num-/used-
	hg_node()
	{
		heavy = vector<unsigned char>(landa_h, 0);
		usage = 0;
		//here we withdraw the num2 because it can be inferred
		//usage += 15;
		//all level 2 initially
		//usage += (2<<8);
		//usage += (1<<16);
	}
	void levelup(int level, int f)
	{
		double ran = 1.0 * rand() / RAND_MAX;
		switch(level)
		{
			case 1:
				{
					if (ran > c1)return;
					int num3 = (usage>>8) & 15;
					int num4 = (usage>>16) & 15;
					int num2 = 8 - num4*2 - num3*3/2;
					int usage2 = usage & 255;
					int start = 0;
					int end = start + num2*2;
					if(usage2 < num2) {//exist empty space
						for (int i = start; i < end; i += 2) {
							if (i >= landa_h) printf("error warning!\n");
							if (heavy[i] == 0) {
								heavy[i] = f&255;
								heavy[i+1] = 1;
								usage += 1;
								return;
							}
						}
					}
					else {//no empty space
						//find weakest guardian
						if (num2 == 0) return;
						int min_f = -1;
						int min_fq = -1;
						for (int i = start; i < end; i += 2) {
							if (i >= landa_h) printf("error warning!\n");
							if (min_f == -1) {
								min_f = i;
								min_fq = heavy[i+1];
							}
							else if (heavy[i+1] < min_fq) {
								min_f = i;
								min_fq = heavy[i+1];
							}
						}
						//exponential decay
						if (min_f == -1 || min_fq < 0) printf("minus 1 warning!\n");
						if (ran < pow(b, min_fq * -1)) {
							heavy[min_f+1] -= 1; 
							if(heavy[min_f+1] <= 0) {
								heavy[min_f] = (f&255);
								heavy[min_f+1] = 1;
							}
						}
					}
					break;
				}
			case 2:
				{
					if (ran > c2) return;
					int num3 = (usage>>8) & 15;
					int num4 = (usage>>16) & 15;
					int num2 = 8 - num4*2 - num3*3/2;
					int usage2 = usage & 255;
					int usage3 = (usage>>12) & 15;
					//cout<<"level 3"<<endl;
					if (num3 == usage3 && num2 > 3) {
						int usage4 = (usage>>20) & 15;
						num3 += 2;
						num2 -= 3;
						int rest = 0;
						if (usage2 > num2) {
							rest = usage2 - num2;
							usage2 = num2;
						}
						//rest == 0: nothing happen
						//rest > 0: kill minimum rest entreis
						if (rest) {
							vector<int> weaks(rest, -1);
							vector<int> widx(rest, -1);
							for (int i = 0; i < (num2+3)*2; i += 2) {
								if (i >= landa_h) printf("error warning!\n");
								for (int j = 0; j < rest; j++) {
									if (widx[j] == -1) {
										widx[j] = i;
										weaks[j] = heavy[i+1];
										break;
									}
									else if (heavy[i+1] < weaks[j] && heavy[i+1] > 0) {
										for(int l = rest - 1; l > j; l--) {
											widx[l] = widx[l-1];
											weaks[l] = weaks[l-1];
										}
										widx[j] = i;
										weaks[j] = heavy[i+1];
										break;
									}
								}
							}
							int kill = 0;
							sort(widx.begin(), widx.end());
							for (int i = widx[kill]; i < (num2+3)*2; i += 2) {
								if (i >= landa_h) printf("error warning!\n");
								if (i == widx[kill]) {
									kill++;
									heavy[i] = 0;
									heavy[i+1] = 0;
									continue;
								}
								else if (kill) {
									heavy[i-kill*2] = heavy[i];
									heavy[i+1-kill*2] = heavy[i+1];
									heavy[i] = 0;
									heavy[i+1] = 0;
								}
							}
						}
						
						usage = 0;
						usage += usage2;
						usage += (num3<<8);
						usage += (usage3<<12);
						usage += (num4<<16);
						usage += (usage4<<20);
					}
					
					int start = num2*2;
					int end = start + num3*3;
					if (usage3 < num3) {//exist empty space
						for (int i = start; i < end; i += 3) {
							if (i >= landa_h) printf("error warning!\n");
							if (heavy[i] == 0) {
								f &= 4095; 
								heavy[i] = (f>>4);
								heavy[i+1] = ((f&15)<<4) + 1;
								heavy[i+2] = 0;

								usage += (1<<12);
								return;
							}
						}
						cout<<"error warning 3!"<<endl;
					}
					else {//no empty space
						//find weakest guardian
						int min_f = -1;
						int min_fq = -1;
						for (int i = start; i < end; i += 2) {
							if (i >= landa_h) printf("error warning!\n");
							int freq = ((int)(heavy[i+1]&15)<<8) + heavy[i+2];
							if (min_f == -1) {
								min_f = i;
								min_fq = freq;
							}
							else if (freq < min_fq && freq>0) {
								min_f = i;
								min_fq = freq;
							}
						}
						//exponential decay
						if (min_f == -1 || min_fq < 0) printf("minus 2 warning!\n");
						if (ran < pow(b, min_fq * -1)) {
							min_fq -= 1; 
							if(min_fq <= 255) {
								f &= 4095; 
								heavy[min_f] = (f>>4);
								heavy[min_f+1] = ((f&15)<<4) + 1;
								heavy[min_f+2] = 0;

							}
							else {
								heavy[min_f+1] = (heavy[min_f+1]&240)+(min_fq>>8);
								heavy[min_f+2] = (min_fq&255); 
							}
						}
					}
					break;
				}
			case 3:
				{
					if (ran > c3) return;
					int num3 = (usage>>8) & 15;
					int num4 = (usage>>16) & 15;
					int num2 = 8 - num4*2 - num3*3/2;
					int usage2 = usage & 255;
					int usage4 = (usage>>20) & 15;
					//cout<<"level 4"<<endl;
					
					if (num4 == usage4 && num2 > 2) {
						num4 += 1;
						int usage3 = (usage>>12) & 15;
						num2 -= 2;
						int rest = 0;
						if (usage2 > num2) {
							rest = usage2 - num2;
							usage2 = num2;
						}
						//rest == 0: nothing happen

						//rest > 0: kill minimum rest entreis
						if (rest) {
							vector<int> weaks(rest, -1);
							vector<int> widx(rest, -1);
							for (int i = 0; i < (num2+2)*2; i += 2) {
								if (i >= landa_h) printf("error warning!\n");
								for (int j = 0; j < rest; j++) {
									if (widx[j] == -1) {
										widx[j] = i;
										weaks[j] = heavy[i+1];
										break;
									}
									else if (heavy[i+1] < weaks[j] && heavy[i+1] > 0) {
										for(int l = rest-1; l>j; l--) {
											widx[l] = widx[l-1];
											weaks[l] = weaks[l-1];
										}
										widx[j] = i;
										weaks[j] = heavy[i+1];
										break;
									}
								}
							}
							int kill = 0;
							sort(widx.begin(), widx.end());
							for (int i = widx[kill]; i< (num2+3)*2; i += 2) {
								if (i >= landa_h) printf("error warning!\n");
								if (i == widx[kill]) {
									kill++;
									heavy[i] = 0;
									heavy[i+1] = 0;
									continue;
								}
								else if(kill) {
									heavy[i-kill*2] = heavy[i];
									heavy[i+1-kill*2] = heavy[i+1];
									heavy[i] = 0;
									heavy[i+1] = 0;
								}
							}
						}
						for (int i = (num2+2)*2; i < (num2+2)*2+num3*3; i += 3) {
							if (i >= landa_h) printf("error warning!\n");
							heavy[i-4] = heavy[i];
							heavy[i+1-4] = heavy[i+1];
							heavy[i+2-4] = heavy[i+2];
							heavy[i] = 0;
							heavy[i+1] = 0;
							heavy[i+2] = 0;
						}
						
						usage = 0;
						usage += usage2;
						usage += (num3<<8);
						usage += (usage3<<12);
						usage += (num4<<16);
						usage += (usage4<<20);
					}
					
					int start = num2*2 + num3*3;
					int end = start + num4*4;
					if (usage4 < num4) {//exist empty space
						for (int i = start;i < end;i += 4) {
							if (i >= landa_h) printf("error warning!\n");
							if (heavy[i] == 0) {
								heavy[i] = (f>>8);
								heavy[i+1] = f&255;
								heavy[i+2] = 16;

								usage += (1<<20);
								return;
							}
							cout<<"error warning 4!"<<endl;
						}
					}
					else {//no empty space
						if (num4 == 0) return;
						//find weakest guardian
						int min_f = -1;
						int min_fq = -1;
						for (int i = start; i < end; i += 2) {
							if (i >= landa_h) printf("error warning!\n");
							int freq = ((int)heavy[i+2]<<8) + heavy[i+3];
							if (min_f == -1) {
								min_f = i;
								min_fq = freq;
							}
							else if (freq < min_fq && freq > 0) {
								min_f = i;
								min_fq = freq;
							}
						}
						//exponential decay
						if (min_f == -1 || min_fq < 0) printf("minus 3 warning!\n");
						if (ran < pow(b, min_fq * -1)) {
							min_fq -= 1; 
							//cout<<"level 4 decay result: "<<min_fq<<endl;
							if (min_fq <= 4095) {
								heavy[min_f] = (f>>8);
								heavy[min_f+1] = f&255;
								heavy[min_f+2] = 16;
								heavy[min_f+3] = 0;
							}
							else {
								heavy[min_f+2] = (min_fq>>8);
								heavy[min_f+3] = (min_fq&255); 
								//cout<<"check level 4: "<<heavy[min_f+2]<<endl;
							}
						}
					}
					break;
				}
			default:break;
		}
		return; 
	}
	void insert(unsigned short f, int hash)
	{
		//if exist a flow
		int num3 = (usage>>8) & 15;
		int num4 = (usage>>16) & 15;
		int num2 = 8 - num4*2 - num3*3/2;
		int usage2 = usage & 255;
		//level 4
		int start = num2*2+num3*3;
		int end = start + num4*4;
		for(int i = start; i < end; i += 4) {
			if (i >= landa_h) printf("error warning!\n");
			unsigned short e = ((unsigned short)heavy[i]<<8) + heavy[i+1];
			if (e==f) {
				if (heavy[i+3] < 255) heavy[i+3]++;
				else if (heavy[i+2] != 255) {
					heavy[i+2]++;
					heavy[i+3] = 0;
				}
				else {
					levelup(4, f);
				}
				return;
			}
		}
		//level 3
		start = num2*2;
		end = start + num3*3;
		for (int i = start; i < end; i+=3) {
			if (i >= landa_h) printf("error warning!\n");
			unsigned short e = ((unsigned short)heavy[i]<<4) + (heavy[i+1]>>4);
			if (e == (f&4095)) {
				if (heavy[i+2] < 255) heavy[i+2]++;
				else if ((heavy[i+1] & 15) != 15) {
					heavy[i+1]++;
					heavy[i+2] = 0;
				}
				else {
					levelup(3, f);
				}
				return;
			}
		}
		//level 2
		start = 0;
		end = start + num2*2;
		for (int i = start; i < end; i+=2) {
			if (i >= landa_h) printf("error warning!\n");
			unsigned short e = heavy[i];
			if (e == (f&255)) {
				if (heavy[i+1] < 255) heavy[i+1]++;
				else {
					levelup(2, f);
				}
				return;
			}
		}
		
		//no existing flow
		levelup(1, f);
	}
	int query(unsigned short f, int hash)
	{
		int num3 = (usage>>8) & 15;
		int num4 = (usage>>16) & 15;
		int num2 = 8 - num4*2 - num3*3/2;
		int usage2 = usage & 255;
		//level 4
		int start = num2*2+num3*3;
		int end = start + num4*4;
		for (int i = start;i < end; i += 4) {
			unsigned short e = ((unsigned short)heavy[i]<<8) + heavy[i+1];
			if (e == f) {
				return ((int)heavy[i+2]<<8) + heavy[i+3];
			}
		}
		//level 3
		start = num2*2;
		end = start + num3*3;
		for (int i = start; i < end; i += 3) {
			unsigned short e = ((unsigned short)heavy[i]<<4) + (heavy[i+1]>>4);
			if (e == (f&4095)) {
				return ((int)(heavy[i+1]&15)<<8) + heavy[i+2];
			}
		}
		//level 2
		start = 0;
		end = start + num2*2;
		for (int i = start; i < end; i += 2) {
			unsigned short e = heavy[i];
			if (e == (f&255)) {
				return heavy[i+1];
			}
		}
		
		//no existing flow
		return 0;
	}
};

void ReadInTraces(const char* trace_prefix)
{
	for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; datafileCnt++)
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

bool cmp1(pair<int,int>p1, pair<int,int>p2)
{
	return p1.first > p2.first;
}

bool cmp2(pair<int,int>p1, pair<int,int>p2)
{
	return p1.second > p2.second;
}

int main()
{
	ReadInTraces("./");
	//ReadInTraces("./data/CAIDA/");
	//ReadInTraces("./data/webpage/");
	//ofstream out("./dms4.txt");
	//ofstream state("./state4.txt");

	//out<<"memory: "<<HEAVY_MEM<<endl;
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
			vector<hg_node>hg(BUCKET_NUM);
			for (int i = 0; i < packet_cnt; ++i)
			{
				int hash = BKDRHash(keys[i], 13); //CAIDA
				//int hash = BKDRHash(keys[i], 8); //webpage
				hg[hash % BUCKET_NUM].insert(finger_print(hash), hash);	
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
				gb_cnt[hash] += 1;
				lc_cnt[hash] += 1;
				if (hit_cnt.count(hash) == 0) hit_cnt[hash] = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
				hg[hash % BUCKET_NUM].insert(finger_print(hash), hash);
				//cout<<i<<"th insertion"<<endl;
				//if (i % 100000 == 0)printf("flow frequency:%d\n", hg[hash % BUCKET_NUM].query(finger_print(hash), hash));
				//hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
				if (i && i % window == 0) {
					//heavy hitter
					if(1) {
						double th = window * hh;
						int tp = 0, fp =0, tn = 0, fn = 0;
						for(auto it:gb_cnt) {
							bool f1 = 0, f2 = 0;
							if (it.second >= th) f1 = 1;
							int hash = it.first;
							int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
							if (efreq >= th) f2 = 1;
							if (f1 && f2) tp++;
							else if (f1 && !f2) fn++;
							else if (!f1 && f2) fp++;
							else if (!f1 && !f2) tn++;
						}
						double recall = (double)tp / (tp + fn);
						double precision = (double)tp / (tp + fp);
						double fscore = 2*(recall * precision) / (recall + precision);
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
							int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash) - hit_cnt[hash];
							ph_cnt[hash] = efreq;
							if (efreq >= th) f2 = 1;
							if (f1 && f2) tp++;
							else if (f1 && !f2) fn++;
							else if (!f1 && f2) fp++;
							else if (!f1 && !f2) tn++;
						}
						double recall = (double)tp / (tp + fn);
						double precision = (double)tp / (tp + fp);
						double fscore = 2*(recall * precision) / (recall + precision);
						//cout<<recall<<" "<<precision<<endl;
						//out<<i/window <<"th heavy hitter FSOCRE:"<<fscore<<endl;
						gb_heavy_changer[i/window-1] += fscore;
						hc_cnt[i/window-1] += 1;
					}
					lc_cnt = unordered_map<int,int>();
					hit_cnt = unordered_map<int,int>();
				}
			}
			
			vector<pair<int,int>>topk;
			for (auto it:gb_cnt) {
				topk.push_back(make_pair(it.first, it.second));
			}
			sort(topk.begin(), topk.end(), cmp2);
			//AAE
			double AAE = 0;
			for (int i = 0; i < topk.size(); i++) {
				int hash = topk[i].first;
				int efq = hg[hash%BUCKET_NUM].query(finger_print(hash), hash);
				AAE += (double)abs(topk[i].second - efq);
			}
			AAE /= gb_cnt.size();
			cout<<"AAE: "<<AAE<<endl;
			gb_AAE += AAE;
			//ARE
			double ARE = 0;
			for (int i = 0; i < topk.size(); i++) {
				int hash = topk[i].first;
				int efq = hg[hash%BUCKET_NUM].query(finger_print(hash), hash);
				ARE += (double)abs(topk[i].second - efq)/topk[i].second;
			}
			ARE /= gb_cnt.size();
			cout<<"ARE: "<<ARE<<endl;
			gb_ARE += ARE;
			//Topk ARE
			double kARE = 0;
			for (int i = 0; i < k; i++) {
				int hash = topk[i].first;
				int efq = hg[hash%BUCKET_NUM].query(finger_print(hash), hash);
				//printf("real freq: %d, estimated freq: %d\n", topk[i].second, efq);
				if (topk[i].second > 255 && (efq < topk[i].second/2 || efq/2 > topk[i].second)) {
					//int usage2 = (hg[hash % BUCKET_NUM].usage)&255;
					//int num3 = (hg[hash % BUCKET_NUM].usage>>8)&15;
					//int usage3 = (hg[hash % BUCKET_NUM].usage>>12)&15;
					//int num4 = (hg[hash % BUCKET_NUM].usage>>16)&15;
					//int usage4 = (hg[hash % BUCKET_NUM].usage>>20)&15;
					//state<<"severe error: "<<topk[i].second<<" "<<efq<<endl;
					//state<<"corresponding state: "<<usage2<<" "<<usage3<<" "<<num3<<" "<<usage4<<" "<<num4<<endl;
				}
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
				int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
				freq_e[efreq] += 1;
				max_freq = max(max_freq, it.second);
				max_freq = max(max_freq, efreq);
			}
			double wmre = 0, wmd = 0;
			for(int i = 1; i <= max_freq; i++) {
				wmre += (double)abs(freq[i] - freq_e[i]);
				wmd += ((double)freq[i] + freq_e[i])/2;
			}
			//out<<"WMRE: "<<wmre/wmd<<endl;
			cout<<"WMRE: "<<wmre/wmd<<endl;
			gb_WMRE += wmre/wmd;
			//entropy
			int flow_num = gb_cnt.size();
			cout<<"flow_num:"<<flow_num<<endl;
			double e = 0, ee = 0;
			for(int i = 1; i <= max_freq; i++) {
				e += freq[i]?-1*((double)i*freq[i]/flow_num)*log2((double)freq[i]/flow_num):0;
				ee += freq_e[i]?-1*((double)i*freq_e[i]/flow_num)*log2((double)freq_e[i]/flow_num):0;
			}
			//out<<"entropy ARE: "<<fabs(e-ee)/e<<endl;
			cout<<"entropy ARE: "<<fabs(e-ee)/e<<endl;
			//out<<"real entropy:"<<e<<endl; 
			gb_entropy += fabs(e-ee)/e;
			//FSCORE
			unordered_map<int,bool>ef;
			double threshold = packet_cnt * hh;
			int tp = 0, fp =0, tn = 0, fn = 0;
			for (auto it:gb_cnt) {
				bool f1 = 0, f2 = 0;
				if (it.second >= threshold) f1 = 1;
				int hash = it.first;
				int efreq = hg[hash % BUCKET_NUM].query(finger_print(hash), hash);
				if (efreq >= threshold) f2 = 1;
				if (f1 && f2) tp++;
				else if (f1 && !f2) fn++;
				else if (!f1 && f2) fp++;
				else if (!f1 && !f2) tn++;
			}
			double recall = (double)tp / (tp + fn);
			double precision = (double)tp / (tp + fp);
			double fscore = 2*(recall * precision) / (recall + precision);
			//cout<<recall<<" "<<precision<<endl;
			//out<<"Total FSOCRE:"<<fscore<<endl;
			cout<<"Total FSOCRE:"<<fscore<<endl;
			gb_fscore += fscore;
			
			//state check
			
			/*for(int i = 0; i<BUCKET_NUM; i++)
			{
				int usage3 = (hg[i].usage>>12)&15;
				int usage4 = (hg[i].usage>>20)&15;
				state<<i<<"th bucket state:"<<usage3;
				state<<" "<<usage4<<endl;
			}
			cout<<"finish state check\n"<<endl;*/

			/* free memory */
			for (int i = 0; i < packet_cnt; ++i)
				delete[] keys[i];
			delete[] keys;
			cout<<"finish free\n"<<endl;
		}
	}
	cout<<"heavy hitter FSCORE:"<<endl;
	for(int i = 0; i < 10; i++) {
		if (hh_cnt[i] > 0) cout<<gb_heavy_hitter[i] / hh_cnt[i]<<endl;
	}
	cout<<"heavy changer FSCORE:"<<endl;
	for (int i = 0; i < 10; i++) {
		if (hc_cnt[i] > 0) cout<<gb_heavy_changer[i] / hc_cnt[i]<<endl;
	}
	cout<<"AAE: "<<gb_AAE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"ARE: "<<gb_ARE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"Topk ARE: "<<gb_kARE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"WMRE: "<<gb_WMRE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"entropy ARE: "<<gb_entropy / (END_FILE_NO * test_cycles)<<endl;
	cout<<"Total FSOCRE: "<<gb_fscore / (END_FILE_NO * test_cycles)<<endl;
	cout<<"throughput: "<<gb_throughput / (END_FILE_NO * test_cycles)<<endl;
	cout<<endl;
}
