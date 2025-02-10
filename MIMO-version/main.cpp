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
#include <time.h>
#include <fstream>
#include "gurobi_c++.h"

#include "mimosketch.h"
#include "countsketch.h"
#include "cmsketch.h"
#include "func.h"

using namespace std;

#define test_cycles 10
#define epoch 10
#define range 7

#define START_FILE_NO 1
#define END_FILE_NO 1 //demo

#define MIMO 0
//#define CS 1
//#define CM 2

struct FIVE_TUPLE { char key[13]; }; //demo

typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];
static int HEAVY_MEM = 200 * 1024;

#if defined (MIMO)
	vector<vector<hg_node_mimo>>hg(range);
#elif defined (CS)
	vector<vector<hg_node_cs>>hg(range);
#elif defined (CM)
	vector<vector<hg_node_cm>>hg(range);
#else
#endif

void ReadInTraces(const char* trace_prefix)
{
	for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		char datafileName[100];
		sprintf(datafileName, "demo.dat"); //demo
		FILE* fin = fopen(datafileName, "rb");
		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt - 1].clear();

		int cnt = 0;
		while (fread(&tmp_five_tuple, 1, 13, fin) == 13 && cnt <= 1800000) { //demo
			traces[datafileCnt - 1].push_back(tmp_five_tuple);
			cnt++;
		}
		fclose(fin);
		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt - 1].size());
	}
	printf("\n");
}

double MeanQuery(const char* key, std::vector<int>opt)
{
	int query[5];
	for (int l = 0; l < 5; l++) {
		int j = opt[l];
		#if defined (MIMO)
			query[l] = hg[j][MurmurHash(key, 13, j)%BUCKET_NUM_MIMO].Query(MurmurHash(key, 13, 101+j)&1, MurmurHash(key, 13, 11+j)&255); //demo
		#elif defined (CS)
			int tmp[HASH_NUM];
			for (int i = 0; i < HASH_NUM; i++)
				tmp[i] = hg[j][MurmurHash(key, 13, j+i)%BUCKET_NUM_CS].Query(MurmurHash(key, 13, 101+j)&1); //demo
			sort(tmp, tmp + HASH_NUM);
			query[l] = tmp[HASH_NUM/2];
		#elif defined (CM)
			int tmp[HASH_NUM];
			for (int i = 0; i < HASH_NUM; i++)
				tmp[i] = hg[j][MurmurHash(key, 13, j+i)%BUCKET_NUM_CM].Query(); //demo
			sort(tmp, tmp + HASH_NUM);
			query[l] = tmp[0];
		#endif
		if (opt.size() == 1) break;
		if (opt.size() == 3 && l == 2) break;
	}
	sort(query, query + opt.size());
	if (opt.size() == 1) return max(query[0], 0);
	if (opt.size() == 5) return max(query[2], 0);
	return max(query[1], 0);
}

int main()
{
	for (int i = 0; i < range; i++) {
		#if defined (MIMO)
			hg[i].resize(BUCKET_NUM_MIMO);
		#elif defined (CS)
			hg[i].resize(BUCKET_NUM_CS);
		#elif defined (CM)
			hg[i].resize(BUCKET_NUM_CM);
		#else
			std::cerr<<"Please select an algorithm!"<<std::endl;
			exit(-1);
		#endif
	}

	ReadInTraces("./"); //demo
	
	cout<<"memory: "<<HEAVY_MEM<<endl;
	double gb_AAE1 = 0;
	double gb_ARE1 = 0;
	double gb_AAE5 = 0;
	double gb_ARE5 = 0;
	double gb_AAE = 0;
	double gb_ARE = 0;
	double gb_oAAE = 0;
	double gb_oARE = 0;
	double gb_otime = 0;
	
	for (int i = 0; i < test_cycles; i++) {	
		srand((unsigned)time(NULL));
		for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; datafileCnt++) {
			unordered_map<string,int>gb_cnt;
			
			int packet_cnt = (int)traces[datafileCnt - 1].size();
			
			char** keys = new char * [(int)traces[datafileCnt - 1].size()];
			for (int i = 0; i < (int)traces[datafileCnt - 1].size(); i++) {
				keys[i] = new char[13]; //demo
				memcpy(keys[i], traces[datafileCnt - 1][i].key, 13); //demo
			}

			for (int i = 0; i < packet_cnt; i++) {
				string str(keys[i], 13); //demo
				gb_cnt[str] += 1;
			}
			
			vector<pair<string,int>>topk;
			for (auto it:gb_cnt) {
				topk.push_back(make_pair(it.first, it.second));
			}
			sort(topk.begin(), topk.end(), cmp2);
			
			unordered_map<string, vector<int>>rnd1;
			unordered_map<string, vector<int>>rnd5;
			unordered_map<string, vector<int>>rnd;

			//------------route generator------------
			vector<vector<int>> route(topk.size());
			for (int i = 0; i < topk.size(); i++) {
				int tmp=rand();
				int choice1[4] = {0,13,22,27};
				int choice2[8] = {1,2,7,8,23,24,25,26};
				int choice3[16] = {3,4,9,10,5,6,11,12,14,15,18,19,16,17,20,21};
				if (tmp%100<70) tmp = choice1[rand()%4];
				else if (tmp%100<85) tmp = choice2[rand()%8];
				else tmp = choice3[rand()%16];
				switch(tmp) {
					case 0: {route[i].resize(1, 0); break;}
					case 1:
					case 2:
					case 7:
					case 8: {route[i].resize(3); route[i][0] = 0; route[i][1] = 1; route[i][2] = 2; break;}
					case 3:
					case 4:
					case 9:
					case 10: {route[i].resize(5); route[i][0] = 0; route[i][1] = 1;
					route[i][2] = 3; route[i][3] = 4; route[i][4] = 5; break;}
					case 5:
					case 6:
					case 11:
					case 12: {route[i].resize(5); route[i][0] = 0; route[i][1] = 1;
					route[i][2] = 3; route[i][3] = 5; route[i][4] = 6; break;}
					case 13: {route[i].resize(1, 2); break;}
					case 14:
					case 15:
					case 18:
					case 19: {route[i].resize(5); route[i][0] = 1; route[i][1] = 2;
					route[i][2] = 3; route[i][3] = 4; route[i][4] = 5; break;}
					case 16:
					case 17:
					case 20:
					case 21: {route[i].resize(5); route[i][0] = 1; route[i][1] = 2;
					route[i][2] = 3; route[i][3] = 5; route[i][4] = 6; break;}
					case 22: {route[i].resize(1, 4); break;}
					case 23:
					case 24:
					case 25:
					case 26: {route[i].resize(3); route[i][0] = 4; route[i][1] = 5; route[i][2] = 6; break;}
					case 27: {route[i].resize(1, 6); break;}
					default: break;
				}
			}

			//------------baseline 1: randomly select 1 node------------
			for (int i = 0; i < topk.size(); i++) {
				rnd1[topk[i].first].resize(1);
				random_shuffle(route[i].begin(), route[i].end());
				rnd1[topk[i].first][0] = route[i][0];
			}

			for (int i = 0; i < range; i++) {
				#if defined (MIMO)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_mimo());
				#elif defined (CS)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cs());
				#elif defined (CM)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cm());
				#endif
			}
			for (int i = 0; i < packet_cnt; i++) {
				string str(keys[i], 13); //demo
				int j = rnd1[str][0];
				#if defined (MIMO)
                    hg[j][MurmurHash(keys[i], 13, j)%BUCKET_NUM_MIMO].Insert(MurmurHash(keys[i], 13, 101+j)&1, MurmurHash(keys[i], 13, 11+j)&255); //demo
				#elif defined (CS)
					for (int l = 0; l < HASH_NUM; l++)
						hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CS].Insert(MurmurHash(keys[i], 13, 101+j)&1); //demo
				#elif defined (CM)
					for (int l = 0; l < HASH_NUM; l++)
						hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CM].Insert(); //demo
				#endif
			}

			//AAE1
			double AAE1 = 0;
			for (int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, rnd1[str]);
				AAE1 += (double)abs(topk[i].second - efq);
			}
			AAE1 /= topk.size();
			gb_AAE1 += AAE1;
			//ARE1			
			double ARE1 = 0;
			for(int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, rnd1[str]);
				ARE1 += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			ARE1 /= topk.size();
			gb_ARE1 += ARE1;

			//------------baseline 2: select all the candidate nodes------------
			for (int i = 0; i < topk.size(); i++) {
				rnd5[topk[i].first].resize(route[i].size());
				rnd5[topk[i].first].assign(route[i].begin(), route[i].end());
			}
			
			for (int i = 0; i < range; i++) {
				#if defined (MIMO)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_mimo());
				#elif defined (CS)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cs());
				#elif defined (CM)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cm());
				#endif
			}
			for (int i = 0; i < packet_cnt; i++) {
				string str(keys[i], 13); //demo
				for (int l = 0; l < rnd5[str].size(); l++) {
					int j = rnd5[str][l];
					#if defined (MIMO)
						hg[j][MurmurHash(keys[i], 13, j)%BUCKET_NUM_MIMO].Insert(MurmurHash(keys[i], 13, 101+j)&1, MurmurHash(keys[i], 13, 11+j)&255); //demo
					#elif defined (CS)
						for (int l = 0; l < HASH_NUM; l++)
							hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CS].Insert(MurmurHash(keys[i], 13, 101+j)&1); //demo
					#elif defined (CM)
						for (int l = 0; l < HASH_NUM; l++)
							hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CM].Insert(); //demo
					#endif
				}
			}

			//AAE5
			double AAE5 = 0;
			for (int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, rnd5[str]);
				AAE5 += (double)abs(topk[i].second - efq);
			}
			AAE5 /= topk.size();
			gb_AAE5 += AAE5;
			//ARE5			
			double ARE5 = 0;
			for(int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, rnd5[str]);
				ARE5 += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			ARE5 /= topk.size();
			gb_ARE5 += ARE5;

			//------------baseline 3: randomly select 3 nodes------------
			for (int i = 0; i < topk.size(); i++) {
				rnd[topk[i].first].resize(min((int)route[i].size(), 3));
				if (route[i].size() <= 3) rnd[topk[i].first].assign(route[i].begin(), route[i].end());
				else {
					random_shuffle(route[i].begin(), route[i].end());
					rnd[topk[i].first][0] = route[i][0];
					rnd[topk[i].first][1] = route[i][1];
					rnd[topk[i].first][2] = route[i][2];
				}
			}
			
			for (int i = 0; i < range; i++) {
				#if defined (MIMO)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_mimo());
				#elif defined (CS)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cs());
				#elif defined (CM)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cm());
				#endif
			}
			for (int i = 0; i < packet_cnt; i++) {
				string str(keys[i], 13); //demo
				for (int l = 0; l < 3; l++) {
					int j = rnd[str][l];
					#if defined (MIMO)
						hg[j][MurmurHash(keys[i], 13, j)%BUCKET_NUM_MIMO].Insert(MurmurHash(keys[i], 13, 101+j)&1, MurmurHash(keys[i], 13, 11+j)&255); //demo
					#elif defined (CS)
						for (int l = 0; l < HASH_NUM; l++)
							hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CS].Insert(MurmurHash(keys[i], 13, 101+j)&1); //demo
					#elif defined (CM)
						for (int l = 0; l < HASH_NUM; l++)
							hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CM].Insert(); //demo
					#endif
					if (rnd[str].size() == 1) break;
				}
			}
			//AAE
			double AAE = 0;
			for (int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, rnd[str]);
				AAE += (double)abs(topk[i].second - efq);
			}
			AAE /= topk.size();
			gb_AAE += AAE;
			//ARE			
			double ARE = 0;
			for(int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, rnd[str]);
				ARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			ARE /= topk.size();
			gb_ARE += ARE;

			//------------solution: MIP-optimally select 3 nodes------------
			unordered_map<string, vector<int>>opt;
			clock_t start = clock();
			try{
				GRBEnv env = GRBEnv(true);
				env.set("LogFile", "main.log");
				env.start();
				GRBModel model = GRBModel(env);
				GRBVar x[topk.size()*range];
				for (int i = 0; i < topk.size()*range; i++)
					x[i] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
				GRBVar obj = model.addVar(0.0, topk.size()*range, 0.0, GRB_INTEGER);
				model.setObjective(1*obj, GRB_MINIMIZE);
				for (int j = 0; j < range; j++) {
					GRBLinExpr constrain = 0.0;
					for (int i = 0; i < topk.size(); i++)
						constrain += x[i*range+j];
					model.addConstr(constrain, GRB_LESS_EQUAL, obj);
				}
				for (int i = 0; i < topk.size(); i++) {
					if (route[i].size() == 1)
						model.addConstr(x[i*range+route[i][0]] == 1);
					else
						model.addConstr(x[i*range]+x[i*range+1]+x[i*range+2]+x[i*range+3]+x[i*range+4]+x[i*range+5]+x[i*range+6] == 3);
					for (int j = 0; j < range; j++)
						if (std::find(route[i].begin(), route[i].end(), j) == route[i].end())
							model.addConstr(x[i*range+j] == 0);
				}
				model.optimize();
				for (int i = 0; i < topk.size(); i++) {
					opt[topk[i].first].resize(3);
					int l = 0;
					for (int j = 0; j < range; j++) {
						if (x[i*range+j].get(GRB_DoubleAttr_X) == 1)
							opt[topk[i].first][l++] = j;
					}
					if (l == 1) opt[topk[i].first].resize(1);
				}
			} catch(GRBException e) {
				cout<<"Error code = "<<e.getErrorCode()<<endl;
				cout<<e.getMessage()<<endl;
			} catch(...) {
				cout<<"Exception during optimization"<<endl;
			}
			clock_t end = clock();
			double otime = (double)(end-start)/CLOCKS_PER_SEC;
			gb_otime += otime;

			for (int i = 0; i < range; i++) {
				#if defined (MIMO)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_mimo());
				#elif defined (CS)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cs());
				#elif defined (CM)
					std::fill(hg[i].begin(), hg[i].end(), hg_node_cm());
				#endif
			}
			for (int i = 0; i < packet_cnt; i++) {
				string str(keys[i], 13); //demo
				for (int l = 0; l < 3; l++) {
					int j = opt[str][l];
					#if defined (MIMO)
						hg[j][MurmurHash(keys[i], 13, j)%BUCKET_NUM_MIMO].Insert(MurmurHash(keys[i], 13, 101+j)&1, MurmurHash(keys[i], 13, 11+j)&255); //demo
					#elif defined (CS)
						for (int l = 0; l < HASH_NUM; l++)
							hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CS].Insert(MurmurHash(keys[i], 13, 101+j)&1); //demo
					#elif defined (CM)
						for (int l = 0; l < HASH_NUM; l++)
							hg[j][MurmurHash(keys[i], 13, j+l)%BUCKET_NUM_CM].Insert(); //demo
					#endif
					if (opt[str].size() == 1) break;
				}
			}
			//AAE
			double oAAE = 0;
			for (int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, opt[str]);
				oAAE += (double)abs(topk[i].second - efq);
			}
			oAAE /= topk.size();
			gb_oAAE += oAAE;
			//ARE			
			double oARE = 0;
			for(int i = 0; i < topk.size(); i++) {
				string str = topk[i].first;
				const char* key = str.c_str();
				int efq = MeanQuery(key, opt[str]);
				oARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			oARE /= topk.size();
			gb_oARE += oARE;
		
			/* free memory */
			for (int i = 0; i < packet_cnt; i++)
				delete[] keys[i];
			delete[] keys;
			cout<<"finish free\n"<<endl;
		}
	}
	cout<<"AAE1: "<<gb_AAE1 / (END_FILE_NO * test_cycles)<<endl;
	cout<<"ARE1: "<<gb_ARE1 / (END_FILE_NO * test_cycles)<<endl;
	cout<<"AAE5: "<<gb_AAE5 / (END_FILE_NO * test_cycles)<<endl;
	cout<<"ARE5: "<<gb_ARE5 / (END_FILE_NO * test_cycles)<<endl;
	cout<<"AAE: "<<gb_AAE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"ARE: "<<gb_ARE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"oAAE: "<<gb_oAAE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"oARE: "<<gb_oARE / (END_FILE_NO * test_cycles)<<endl;
	cout<<"otime: "<<gb_otime / (END_FILE_NO * test_cycles)<<" s"<<endl;
}
