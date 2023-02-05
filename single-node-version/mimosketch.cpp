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
#include <unordered_map>
#include <map>
#include <algorithm>
#include <fstream>

using namespace std;

#define START_FILE_NO 1
#define END_FILE_NO 1 //demo
//#define END_FILE_NO 10 //CAIDA
//#define END_FILE_NO 9 //webpage

#define landa_h 16 
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

inline unsigned int BKDRHash(char* str, int len = 1)
{
	unsigned int seed = 131;
	unsigned int hash = 0;
	for (int i = 0; i < len; i++) {
		hash = hash * seed + (*str++);
	}
	return (hash & 0x7FFFFFFF);
}

inline unsigned int SHash(unsigned int hash)
{
	unsigned int b = 378551;
	unsigned int a = 63689;
	unsigned int res = 0; 
	for (int i = 0; i < 4; i++) {
		res = res * a + ((hash>>(i*8))&0xff);
		a = a * b;
	}
	return res;
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
	vector<int8_t>heavy;
	int usage;//num-/used-/count
	hg_node()
	{
		heavy = vector<int8_t>(landa_h, 0);//landa_h = 16 
		usage = 0;
		//here we withdraw the num2 because it can be inferred
		//all level 2 initially
	}
};

vector<hg_node>hg(BUCKET_NUM);

bool cmp1(pair<pair<int, uint8_t>, int>p1, pair<pair<int, uint8_t>, int>p2)
{
	return abs(p1.second) >= abs(p2.second);
}

bool cmp2(pair<int, int>p1, pair<int, int>p2)
{
	return p1.second > p2.second;
}

void Waving(int hash, int fq, uint8_t f)
{
	int count = hg[hash].usage >> 16;
	count += fq;
	hg[hash].usage = (hg[hash].usage & 0x0000ffff) | (count << 16);
}

void Delete(int level, int hash, int pos, int fq, uint8_t f)
{
	if (level == 2) {
		hg[hash].heavy[pos] = 0;
		hg[hash].heavy[pos+1] = 0;
		hg[hash].usage -= 1;
	}
	else if (level == 3) {
		if (pos == 0 || pos == 5 || pos == 6 || pos == 11) {
			hg[hash].heavy[pos] = 0;
			hg[hash].heavy[pos+1] = 0;
			hg[hash].heavy[pos+2] &= 15;
		}
		else if (pos == 4 || pos == 9 || pos == 10 || pos == 15) {
			pos -= 4;
			hg[hash].heavy[pos+4] = 0;
			hg[hash].heavy[pos+3] = 0;
			hg[hash].heavy[pos+2] &= 0xf0;
		}
		hg[hash].usage -= (1<<8);
	}
	Waving(hash, fq, f);
}

bool Levelup(int level, int hash, int pos, int fq, uint8_t f)
{
	double ran = 1.0 * rand() / RAND_MAX;
	int s[2] = {-1, 1};
	switch(level)
	{
		case 1:
			{
				if (ran > c1) return false;
				int num3 = (hg[hash].usage>>4) & 15;
				int num4 = (hg[hash].usage>>12) & 3;
				int num2 = 8 - num4*3/2 - num3*5/4;
				int usage2 = hg[hash].usage & 15;
				int start = 0;
				int end = start + num2*2;
				if (usage2 < num2) {//exist empty space
					for (int i = start; i < end; i += 2) {
						if (i >= landa_h) printf("Error warning!\n");
						if (hg[hash].heavy[i] == 0) {
							hg[hash].heavy[i] = f;
							hg[hash].heavy[i+1] = fq;
							hg[hash].usage++;
							return true;
						}
					}
				}
				break;
			}
		case 2:
			{
				if (ran > c2) return false;
				int num3 = (hg[hash].usage>>4) & 15;
				int num4 = (hg[hash].usage>>12) & 3;
				int num2 = 8 - num4*3/2 - num3*5/4;
				int usage2 = hg[hash].usage & 15;
				int usage3 = (hg[hash].usage>>8) & 15;
				int usage4 = (hg[hash].usage>>14) & 3;
				if (num3 == usage3 && num2 >= 5) {
					num2 -= 5;
					num3 += 4;
					usage2 = 0;
					usage3 = 0;
					vector<pair<pair<int, uint8_t>, int>>fp_fq;
					for (int i = 0; i < (num2+5)*2; i += 2) {
						if (hg[hash].heavy[i]) {
							if (hg[hash].heavy[i] == f)
								fp_fq.push_back(make_pair(make_pair(i, hg[hash].heavy[i]), fq));
							else 
								fp_fq.push_back(make_pair(make_pair(i, hg[hash].heavy[i]), hg[hash].heavy[i+1]));
						}
					}
					sort(fp_fq.begin(), fp_fq.end(), cmp1);
					if (fp_fq.size() == 8) {
						Waving(hash, fp_fq[7].second, fp_fq[7].first.second);
						fp_fq.erase(fp_fq.end());
					}
					for (int i = num2*2; i < num2*2+num3*5/2; i += 5) {
						if (!fp_fq.empty()) {
							hg[hash].heavy[i] = fp_fq[0].first.second;
							hg[hash].heavy[i+1] = fp_fq[0].second >> 4;
							hg[hash].heavy[i+2] = (fp_fq[0].second&15) << 4;
							fp_fq.erase(fp_fq.begin());
							usage3++;
						}
						else {
							for (int j = i; j < num2*2+num3*5/2; j++)
								hg[hash].heavy[j] = 0;
							break;
						}
						if (!fp_fq.empty()) {
							hg[hash].heavy[i+4] = fp_fq[0].first.second;
							hg[hash].heavy[i+3] = fp_fq[0].second >> 4;
							hg[hash].heavy[i+2] |= (fp_fq[0].second & 15);
							fp_fq.erase(fp_fq.begin());
							usage3++;
						}
						else {
							for (int j = i+3; j < num2*2+num3*5/2; j++)
								hg[hash].heavy[j] = 0;
							break;
						}
					}
					for (int i = 0; i < num2*2; i += 2) {
						if (!fp_fq.empty()) {
							hg[hash].heavy[i] = fp_fq[0].first.second;
							hg[hash].heavy[i+1] = fp_fq[0].second;
							fp_fq.erase(fp_fq.begin());
							usage2++;
						}
						else {
							for (int j = i; j < num2*2; j++)
								hg[hash].heavy[j] = 0;
							break;
						}
					}
					hg[hash].usage &= 0xffff0000;
					hg[hash].usage += usage2;
					hg[hash].usage += (num3<<4);
					hg[hash].usage += (usage3<<8);
					hg[hash].usage += (num4<<12);
					hg[hash].usage += (usage4<<14);
					return true;
				}
				int start = num2*2;
				int end = start + num3*5/2;
				if (usage3 < num3) {//exist empty space
					for (int i = start; i < end; i += 5) {
						if (i >= landa_h) printf("Error warning!\n");
						if (hg[hash].heavy[i] == 0) {
							hg[hash].heavy[i] = f;
							hg[hash].heavy[i+1] = fq >> 4;
							hg[hash].heavy[i+2] = (hg[hash].heavy[i+2]&15) + ((fq&15)<<4);
							hg[hash].usage += (1<<8);
							return true;
						}
						if (hg[hash].heavy[i+4] == 0) {
							hg[hash].heavy[i+4] = f;
							hg[hash].heavy[i+3] = fq >> 4;
							hg[hash].heavy[i+2] = (hg[hash].heavy[i+2]&0xf0) + (fq&15);
							hg[hash].usage += (1<<8);
							return true;
						}
					}
					cout<<"Error warning 3!"<<endl;
				}
				break;
			}
		case 3:
			{
				if (ran > c3) return false;
				int num3 = (hg[hash].usage>>4) & 15;
				int num4 = (hg[hash].usage>>12) & 3;
				int num2 = 8 - num4*3/2 - num3*5/4;
				int usage2 = hg[hash].usage & 15;
				int usage3 = (hg[hash].usage>>8) & 15;
				int usage4 = (hg[hash].usage>>14) & 3;
				if (num4 == usage4 && num2 >= 3) {
					num2 -= 3;
					num4 += 2;
					usage2 = 0;
					usage3 = 0;
					usage4 = 0;
					vector<pair<pair<int, uint8_t>, int>>fp_fq;
					for (int i = 0; i < (num2+3)*2; i += 2) {
						if (hg[hash].heavy[i]) {
							fp_fq.push_back(make_pair(make_pair(i, hg[hash].heavy[i]), hg[hash].heavy[i+1]));
						}
					}
					for (int i = (num2+3)*2; i < (num2+3)*2+num3*5/2; i += 5) {
						if (hg[hash].heavy[i]) {
							if (hg[hash].heavy[i] == f)
								fp_fq.push_back(make_pair(make_pair(i, hg[hash].heavy[i]), fq));
							else
								fp_fq.push_back(make_pair(make_pair(i, hg[hash].heavy[i]), (hg[hash].heavy[i+1]<<4) + ((hg[hash].heavy[i+2]>>4)&0x15)));
						}
						if (hg[hash].heavy[i+4]) {
							if (hg[hash].heavy[i+4] == f)
								fp_fq.push_back(make_pair(make_pair(i+4, hg[hash].heavy[i+4]), fq));
							else
								fp_fq.push_back(make_pair(make_pair(i+4, hg[hash].heavy[i+4]), (hg[hash].heavy[i+3]<<4) + (hg[hash].heavy[i+2]&15)));
						}
					}
					sort(fp_fq.begin(), fp_fq.end(), cmp1);
					if (fp_fq.size() == 7) {
						Waving(hash, fp_fq[6].second, fp_fq[6].first.second);
						fp_fq.erase(fp_fq.end());
					}
					for (int i = num2*2+num3*5/2; i < num2*2+num3*5/2+num4*3; i += 3) {
						if (!fp_fq.empty()) {
							hg[hash].heavy[i] = fp_fq[0].first.second;
							hg[hash].heavy[i+1] = fp_fq[0].second >> 8;
							hg[hash].heavy[i+2] = fp_fq[0].second & 255;
							fp_fq.erase(fp_fq.begin());
							usage4++;
						}
						else {
							for (int j = i; j < num2*2+num3*5/2+num4*3; j++)
								hg[hash].heavy[j] = 0;
							break;
						}
					}
					for (int i = num2*2; i < num2*2+num3*5/2; i += 5) {
						if (!fp_fq.empty()) {
							hg[hash].heavy[i] = fp_fq[0].first.second;
							hg[hash].heavy[i+1] = fp_fq[0].second >> 4;
							hg[hash].heavy[i+2] = (fp_fq[0].second&15) << 4;
							fp_fq.erase(fp_fq.begin());
							usage3++;
						}
						else {
							for (int j = i; j < num2*2+num3*5/2; j++)
								hg[hash].heavy[j] = 0;
							break;
						}
						if (!fp_fq.empty()) {
							hg[hash].heavy[i+4] = fp_fq[0].first.second;
							hg[hash].heavy[i+3] = fp_fq[0].second >> 4;
							hg[hash].heavy[i+2] |= (fp_fq[0].second & 15);
							fp_fq.erase(fp_fq.begin());
							usage3++;
						}
						else {
							for (int j = i+3; j < num2*2+num3*5/2; j++)
								hg[hash].heavy[j] = 0;
							break;
						}
					}
					hg[hash].usage &= 0xffff0000;
					hg[hash].usage += usage2;
					hg[hash].usage += (num3<<4);
					hg[hash].usage += (usage3<<8);
					hg[hash].usage += (num4<<12);
					hg[hash].usage += (usage4<<14);
					return true;
				}					
				int start = num2*2 + num3*5/2;
				int end = start + num4*3;
				if (usage4 < num4) {//exist empty space
					for (int i = start; i < end; i += 3) {
						if (i >= landa_h) printf("Error warning!\n");
						if (hg[hash].heavy[i] == 0) {
							hg[hash].heavy[i] = f;
							hg[hash].heavy[i+1] = fq >> 8;
							hg[hash].heavy[i+2] = fq & 255;
							hg[hash].usage += (1<<14);
							return true;
						}
						cout<<"Error warning 4!"<<endl;
					}
				}
				break;
			}
		default:break;
	}
	return false; 
}

bool Exchange(int level, int hash, int pos, int fq, uint8_t f)
{
	double ran = 1.0 * rand() / RAND_MAX;
	switch(level)
	{
		case 2:
			{
				if (ran > c1) return false;
				int num3 = (hg[hash].usage>>4) & 15;
				int num4 = (hg[hash].usage>>12) & 3;
				int num2 = 8 - num4*3/2 - num3*5/4;
				int usage3 = (hg[hash].usage>>8) & 15;
				int minf = -1;
				uint8_t minfp = -1;
				int minfq = -1;
				if (num3 == 0) return false;
				for (int i = num2*2; i < num2*2+num3*5/2; i += 5) {
					int fq = (hg[hash].heavy[i+1]<<4) + ((hg[hash].heavy[i+2]>>4)&15);
					if (minf == -1) {
						minf = i;
						minfp = hg[hash].heavy[i];
						minfq = fq;
					}
					else {
						if (abs(fq) < abs(minfq)) {
							minf = i;
							minfp = hg[hash].heavy[i];
							minfq = fq;
						}
					}
					fq = (hg[hash].heavy[i+3]<<4) + (hg[hash].heavy[i+2]&15);
					if (minf == -1) {
						minf = i + 4;
						minfp = hg[hash].heavy[i+4];
						minfq = fq;
					}
					else {
						if (abs(fq) < abs(minfq)) {
							minf = i + 4;
							minfp = hg[hash].heavy[i+4];
							minfq = fq;
						}
					}
				}
				if ((minfq <= 127) && (minfq >= -128)) {
					if (minf == 6 || minf == 11) {
						hg[hash].heavy[minf] = f;
						hg[hash].heavy[minf+1] = fq >> 4;
						hg[hash].heavy[minf+2] = (hg[hash].heavy[minf+2]&15) + ((fq&15)<<4);
						hg[hash].heavy[pos] = minfp;
						hg[hash].heavy[pos+1] = minfq;
						return true;
					}
					else if (minf == 10 || minf == 15) {
						minf -= 4;
						hg[hash].heavy[minf+4] = f;
						hg[hash].heavy[minf+3] = fq >> 4;
						hg[hash].heavy[minf+2] = (hg[hash].heavy[minf+2]&0xf0) + (fq&15);
						hg[hash].heavy[pos] = minfp;
						hg[hash].heavy[pos+1] = minfq;
						return true;
					}
				}
				break;
			}
		case 3:
			{
				if (ran > c1) return false;
				int num3 = (hg[hash].usage>>4) & 15;
				int num4 = (hg[hash].usage>>12) & 3;
				int num2 = 8 - num4*3/2 - num3*5/4;
				int usage4 = (hg[hash].usage>>14) & 3;
				int minf = -1;
				uint8_t minfp = -1;
				int minfq = -1;
				if (num4 == 0) return false;
				for (int i = num2*2+num3*5/2; i < num2*2+num3*5/2+num4*3; i += 3) {
					int fq = (hg[hash].heavy[i+1]<<8) + (hg[hash].heavy[i+2]);
					if (minf == -1) {
						minf = i;
						minfp = hg[hash].heavy[i];
						minfq = fq;
					}
					else {
						if (abs(fq) < abs(minfq)) {
							minf = i;
							minfp = hg[hash].heavy[i];
							minfq = fq;
						}
					}
				}
				if ((minfq <= 0x7ff) && (minfq >= -0x800)) {
					hg[hash].heavy[minf] = f;
					hg[hash].heavy[minf+1] = fq >> 8;
					hg[hash].heavy[minf+2] = fq & 255;
					if (pos == 0 || pos == 5) {
						hg[hash].heavy[pos] = minfp;
						hg[hash].heavy[pos+1] = minfq >> 4;
						hg[hash].heavy[pos+2] = (hg[hash].heavy[pos+2]&15) + ((minfq&15)<<4);
						return true;
					}
					else if (pos == 4 || pos == 9) {
						pos -= 4;
						hg[hash].heavy[pos+4] = minfp;
						hg[hash].heavy[pos+3] = minfq >> 4;
						hg[hash].heavy[pos+2] = (hg[hash].heavy[pos+2]&0xf0) + (minfq&15);
						return true;
					}
				}
				break;
			}
		default: break;
	}
	return false;
}

void Insert(int hash, int shash, uint8_t f)
{
	int s[2] = {-1, 1};
	//if exist the flow
	//level 4
	int num3 = (hg[hash].usage>>4) & 15;
	int num4 = (hg[hash].usage>>12) & 3;
	int num2 = 8 - num4*3/2 - num3*5/4;	
	int start = num2*2 + num3*5/2;
	int end = start + num4*3;
	for (int j = start; j < end; j += 3) {
		if (j >= landa_h) printf("Error warning!\n");
		uint8_t e = hg[hash].heavy[j];
		if (e == f) {
			int fq = (hg[hash].heavy[j+1]<<8) + (uint8_t)hg[hash].heavy[j+2] + s[shash];
			hg[hash].heavy[j+1] = fq >> 8;
			hg[hash].heavy[j+2] = fq & 255;
			return;
		}
	}
	//level 3
	start = num2*2;
	end = start + num3*5/2;
	for (int j = start; j < end; j += 5) {
		if (j >= landa_h) printf("Error warning!\n");
		uint8_t e = hg[hash].heavy[j];
		if (e == f) {
			int fq = (hg[hash].heavy[j+1]<<4) + ((hg[hash].heavy[j+2]>>4)&15) + s[shash];
			if ((fq <= 0x7ff) && (fq >= -0x800)) {
				hg[hash].heavy[j+1] = fq >> 4;
				hg[hash].heavy[j+2] = (hg[hash].heavy[j+2]&15) + ((fq&15)<<4);
			}
			else {
				if (Exchange(3, hash, j, fq, f)) return;
				if (Levelup(3, hash, j, fq, f)) return;
				else Delete(3, hash, j, fq, f);
			}
			return;
		}
		e = hg[hash].heavy[j+4];
		if (e == f) {
			int fq = (hg[hash].heavy[j+3]<<4) + (hg[hash].heavy[j+2]&15) + s[shash];
			if ((fq <= 0x7ff) && (fq >= -0x800)) {
			hg[hash].heavy[j+3] = fq >> 4;
				hg[hash].heavy[j+2] = (hg[hash].heavy[j+2]&0xf0) + (fq&15);
			}
			else {
				if (Exchange(3, hash, j+4, fq, f)) return;
				if (Levelup(3, hash, j+4, fq, f)) return;
				else Delete(3, hash, j+4, fq, f);
			}
			return;
		}
	}
	//level 2
	start = 0;
	end = start + num2*2;
	for (int j = start; j < end; j += 2) {
		if (j >= landa_h) printf("Error warning!\n");
		uint8_t e = hg[hash].heavy[j];
		if (e == f) {
			int fq = hg[hash].heavy[j+1] + s[shash];
			if ((fq <= 127) && (fq >= -128))
				hg[hash].heavy[j+1] = fq;
			else {
				if (Exchange(2, hash, j, fq, f)) return;
				if (Levelup(2, hash, j, fq, f)) return;
				else Delete(2, hash, j, fq, f);
			}
			return;
		}
	}
	//no existing flow
	int usage2 = hg[hash].usage & 15;
	if (usage2 < num2) {
		Levelup(1, hash, 0, s[shash], f);
		return;
	}
	//sharing counter
	Waving(hash, s[shash], f);
}

int Query(int hash, int shash, uint8_t f)
{
	int s[2] = {-1, 1};
	//level 4
	int num3 = (hg[hash].usage>>4) & 15;
	int num4 = (hg[hash].usage>>12) & 3;
	int num2 = 8 - num4*3/2 - num3*5/4;	
	int start = num2*2 + num3*5/2;
	int end = start + num4*3;
	for (int j = start; j < end; j += 3) {
		uint8_t e = hg[hash].heavy[j];
		if (e == f) {
			return max(((hg[hash].heavy[j+1]<<8) + (uint8_t)hg[hash].heavy[j+2]) * s[shash], 0);
		}
	}
	//level 3
	start = num2*2;
	end = start + num3*5/2;
	for (int j = start; j < end; j += 5) {
		uint8_t e = hg[hash].heavy[j];
		if (e == f) {
			return max(((hg[hash].heavy[j+1]<<4) + ((hg[hash].heavy[j+2]>>4)&15)) * s[shash], 0);
		}
		e = hg[hash].heavy[j+4];
		if (e == f) {
			return max(((hg[hash].heavy[j+3]<<4) + (hg[hash].heavy[j+2]&15)) * s[shash], 0);
		}
	}
	//level 2
	start = 0;
	end = start + num2*2;
	for (int j = start; j < end; j += 2) {
		uint8_t e = hg[hash].heavy[j];
		if (e == f) {
			return max(hg[hash].heavy[j+1] * s[shash], 0);
		}
	}
	//sharing counter
	int count = hg[hash].usage>>16;
	return max(count*s[shash], 0);
}

void output(int hash, uint8_t f)
{
	int num3 = (hg[hash].usage>>4) & 15;
	int num4 = (hg[hash].usage>>12) & 3;
	int num2 = 8 - num4*3/2 - num3*5/4;
	int start = 0;
	int end = num2*2;
	cout<<hash<<endl;
	cout<<"num2 "<<num2<<endl;
	for (int j = start; j < end; j += 2) {
		uint8_t e = hg[hash].heavy[j];
		cout<<(int)e<<" "<<(int)hg[hash].heavy[j+1]<<endl;
	}
	start = num2*2;
	end = start + num3*5/2;
	cout<<"num3 "<<num3<<endl;
	for (int j = start; j < end; j += 5) {
		uint8_t e = hg[hash].heavy[j];
		cout<<(int)e<<" "<<((int)(hg[hash].heavy[j+1]<<4)+((hg[hash].heavy[j+2]>>4)&15))<<endl;
		e = hg[hash].heavy[j+4];
		cout<<(int)e<<" "<<((int)(hg[hash].heavy[j+3]<<4)+(hg[hash].heavy[j+2]&15))<<endl;
	}
	start = num2*2 + num3*5/2;
	end = start + num4*3;
	cout<<"num4 "<<num4<<endl;
	for (int j = start; j < end; j += 3) {
		uint8_t e = hg[hash].heavy[j];
		cout<<(int)e<<" "<<((int)(hg[hash].heavy[j+1]<<8) + (uint8_t)hg[hash].heavy[j+2])<<endl;
	}
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
		for (int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; datafileCnt++) {
			unordered_map<int,int>gb_cnt;
			unordered_map<int,int>lc_cnt;
			unordered_map<int,int>pre_cnt;
			unordered_map<int,int>hit_cnt;
			unordered_map<int,int>ph_cnt;
			unordered_map<int,int>freq;
			unordered_map<int,int>freq_e;
			map<pair<int, int>, int>ans;
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
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				Insert(hash1, shash, fp);
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
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				gb_cnt[hash] += 1;
				lc_cnt[hash] += 1;
				if (hit_cnt.count(hash) == 0) hit_cnt[hash] = Query(hash1, shash, fp);
				Insert(hash1, shash, fp);
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
							int shash = SHash(hash) & 1;
							int hash1 = hash % BUCKET_NUM;
							int efreq = Query(hash1, shash, fp);
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
						//out<<i/window <<"th heavy hitter FSCORE: "<<fscore<<endl;
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
							int shash = SHash(hash) & 1;
							int hash1 = hash % BUCKET_NUM;
							int efreq = Query(hash1, shash, fp) - hit_cnt[hash];
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
						//out<<i/window <<"th heavy hitter FSCORE: "<<fscore<<endl;
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
				uint8_t fp = finger_print(hash);
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				int efq = Query(hash1, shash, fp);
				AAE += (double)abs(topk[i].second - efq);
				ans[make_pair(topk[i].second,efq)]++;
			}
			AAE /= gb_cnt.size();
			//out<<"AAE: "<<AAE<<endl;
			cout<<"AAE: "<<AAE<<endl;
			gb_AAE += AAE;
			/*double tmpARE = 0;
			int truecount = 0;
			for(auto it:ans) {
				if(it.first.first==it.first.second) truecount+=it.second;
				tmpARE += it.second*(double)abs(it.first.second-it.first.first)/it.first.first;
				cout<<"true: "<<it.first.first<<" estm: "<<it.first.second<<" count: "<<it.second<<" ARE: "<<(double)abs(it.first.second-it.first.first)/it.first.first<<endl;
			}
			cout<<"true count: "<<truecount<<endl;
			cout<<"ARE: "<<tmpARE<<endl;*/
			//ARE			
			double ARE = 0;
			for(int i = 0; i < topk.size(); i++) {
				int hash = topk[i].first;
				uint8_t fp = finger_print(hash);
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				int efq = Query(hash1, shash, fp);
				ARE += (double)abs(topk[i].second - efq) / topk[i].second;
			}
			ARE /= gb_cnt.size();
			//out<<"ARE: "<<ARE<<endl;
			cout<<"ARE: "<<ARE<<endl;
			gb_ARE += ARE;
			//Topk ARE
			double kARE = 0;
			for(int i = 0; i < k; i++) {
				int hash = topk[i].first;
				uint8_t fp = finger_print(hash);
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				int efq = Query(hash1, shash, fp);
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
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				int efreq = Query(hash1, shash, fp);
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
				int shash = SHash(hash) & 1;
				int hash1 = hash % BUCKET_NUM;
				int efreq = Query(hash1, shash, fp);
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
			//out<<"Total FSCORE: "<<fscore<<endl;
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
	cout<<"heavy changer FSCORE: "<<endl;
	for(int i = 0; i < 10; i++) {
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
