#pragma once

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>

#define BUCKET_NUM_MIMO (HEAVY_MEM / 16)
#define landa_h 16
#define c1 1 
#define c2 1
#define c3 1

bool cmp1(std::pair<std::pair<int, uint8_t>, int>p1, std::pair<std::pair<int, uint8_t>, int>p2)
{
	return abs(p1.second) >= abs(p2.second);
}

bool cmp2(std::pair<std::string, int>p1, std::pair<std::string, int>p2)
{
	return p1.second > p2.second;
}

class hg_node_mimo
{
public:
	std::vector<int8_t>heavy;
	int usage;//num-/used-/count

	hg_node_mimo()
	{
		heavy = std::vector<int8_t>(landa_h, 0);//landa_h = 16 
		usage = 0;
		//here we withdraw the num2 because it can be inferred
		//all level 2 initially
	}

	void Waving(int fq, uint8_t f)
	{
		int count = usage >> 16;
		count += fq;
		usage = (usage & 0x0000ffff) | (count << 16);
	}
	
	void Delete(int level, int pos, int fq, uint8_t f)
	{
		if (level == 2) {
			heavy[pos] = 0;
			heavy[pos+1] = 0;
			usage -= 1;
		}
		else if (level == 3) {
			if (pos == 0 || pos == 5 || pos == 6 || pos == 11) {
				heavy[pos] = 0;
				heavy[pos+1] = 0;
				heavy[pos+2] &= 15;
			}
			else if (pos == 4 || pos == 9 || pos == 10 || pos == 15) {
				pos -= 4;
				heavy[pos+4] = 0;
				heavy[pos+3] = 0;
				heavy[pos+2] &= 0xf0;
			}
			usage -= (1<<8);
		}
		Waving(fq, f);
	}
	
	bool Levelup(int level, int pos, int fq, uint8_t f)
	{
		double ran = 1.0 * rand() / RAND_MAX;
		int s[2] = {-1, 1};
		switch(level)
		{
			case 1:
				{
					if (ran > c1) return false;
					int num3 = (usage>>4) & 15;
					int num4 = (usage>>12) & 3;
					int num2 = 8 - num4*3/2 - num3*5/4;
					int usage2 = usage & 15;
					int start = 0;
					int end = start + num2*2;
					if (usage2 < num2) {//exist empty space
						for (int i = start; i < end; i += 2) {
							if (i >= landa_h) printf("Error warning!\n");
							if (heavy[i] == 0) {
								heavy[i] = f;
								heavy[i+1] = fq;
								usage++;
								return true;
							}
						}
					}
					break;
				}
			case 2:
				{
					if (ran > c2) return false;
					int num3 = (usage>>4) & 15;
					int num4 = (usage>>12) & 3;
					int num2 = 8 - num4*3/2 - num3*5/4;
					int usage2 = usage & 15;
					int usage3 = (usage>>8) & 15;
					int usage4 = (usage>>14) & 3;
					if (num3 == usage3 && num2 >= 5) {
						num2 -= 5;
						num3 += 4;
						usage2 = 0;
						usage3 = 0;
						std::vector<std::pair<std::pair<int, uint8_t>, int>>fp_fq;
						for (int i = 0; i < (num2+5)*2; i += 2) {
							if (heavy[i]) {
								if (heavy[i] == f)
									fp_fq.push_back(std::make_pair(std::make_pair(i, heavy[i]), fq));
								else 
									fp_fq.push_back(std::make_pair(std::make_pair(i, heavy[i]), heavy[i+1]));
							}
						}
						sort(fp_fq.begin(), fp_fq.end(), cmp1);
						if (fp_fq.size() == 8) {
							Waving(fp_fq[7].second, fp_fq[7].first.second);
							fp_fq.erase(fp_fq.end());
						}
						for (int i = num2*2; i < num2*2+num3*5/2; i += 5) {
							if (!fp_fq.empty()) {
								heavy[i] = fp_fq[0].first.second;
								heavy[i+1] = fp_fq[0].second >> 4;
								heavy[i+2] = (fp_fq[0].second&15) << 4;
								fp_fq.erase(fp_fq.begin());
								usage3++;
							}
							else {
								for (int j = i; j < num2*2+num3*5/2; j++)
									heavy[j] = 0;
								break;
							}
							if (!fp_fq.empty()) {
								heavy[i+4] = fp_fq[0].first.second;
								heavy[i+3] = fp_fq[0].second >> 4;
								heavy[i+2] |= (fp_fq[0].second & 15);
								fp_fq.erase(fp_fq.begin());
								usage3++;
							}
							else {
								for (int j = i+3; j < num2*2+num3*5/2; j++)
									heavy[j] = 0;
								break;
							}
						}
						for (int i = 0; i < num2*2; i += 2) {
							if (!fp_fq.empty()) {
								heavy[i] = fp_fq[0].first.second;
								heavy[i+1] = fp_fq[0].second;
								fp_fq.erase(fp_fq.begin());
								usage2++;
							}
							else {
								for (int j = i; j < num2*2; j++)
									heavy[j] = 0;
								break;
							}
						}
						usage &= 0xffff0000;
						usage += usage2;
						usage += (num3<<4);
						usage += (usage3<<8);
						usage += (num4<<12);
						usage += (usage4<<14);
						return true;
					}
					int start = num2*2;
					int end = start + num3*5/2;
					if (usage3 < num3) {//exist empty space
						for (int i = start; i < end; i += 5) {
							if (i >= landa_h) printf("Error warning!\n");
							if (heavy[i] == 0) {
								heavy[i] = f;
								heavy[i+1] = fq >> 4;
								heavy[i+2] = (heavy[i+2]&15) + ((fq&15)<<4);
								usage += (1<<8);
								return true;
							}
							if (heavy[i+4] == 0) {
								heavy[i+4] = f;
								heavy[i+3] = fq >> 4;
								heavy[i+2] = (heavy[i+2]&0xf0) + (fq&15);
								usage += (1<<8);
								return true;
							}
						}
						std::cout<<"Error warning 3!"<<std::endl;
					}
					break;
				}
			case 3:
				{
					if (ran > c3) return false;
					int num3 = (usage>>4) & 15;
					int num4 = (usage>>12) & 3;
					int num2 = 8 - num4*3/2 - num3*5/4;
					int usage2 = usage & 15;
					int usage3 = (usage>>8) & 15;
					int usage4 = (usage>>14) & 3;
					if (num4 == usage4 && num2 >= 3) {
						num2 -= 3;
						num4 += 2;
						usage2 = 0;
						usage3 = 0;
						usage4 = 0;
						std::vector<std::pair<std::pair<int, uint8_t>, int>>fp_fq;
						for (int i = 0; i < (num2+3)*2; i += 2) {
							if (heavy[i]) {
								fp_fq.push_back(std::make_pair(std::make_pair(i, heavy[i]), heavy[i+1]));
							}
						}
						for (int i = (num2+3)*2; i < (num2+3)*2+num3*5/2; i += 5) {
							if (heavy[i]) {
								if (heavy[i] == f)
									fp_fq.push_back(std::make_pair(std::make_pair(i, heavy[i]), fq));
								else
									fp_fq.push_back(std::make_pair(std::make_pair(i, heavy[i]), (heavy[i+1]<<4) + ((heavy[i+2]>>4)&0x15)));
							}
							if (heavy[i+4]) {
								if (heavy[i+4] == f)
									fp_fq.push_back(std::make_pair(std::make_pair(i+4, heavy[i+4]), fq));
								else
									fp_fq.push_back(std::make_pair(std::make_pair(i+4, heavy[i+4]), (heavy[i+3]<<4) + (heavy[i+2]&15)));
							}
						}
						sort(fp_fq.begin(), fp_fq.end(), cmp1);
						if (fp_fq.size() == 7) {
							Waving(fp_fq[6].second, fp_fq[6].first.second);
							fp_fq.erase(fp_fq.end());
						}
						for (int i = num2*2+num3*5/2; i < num2*2+num3*5/2+num4*3; i += 3) {
							if (!fp_fq.empty()) {
								heavy[i] = fp_fq[0].first.second;
								heavy[i+1] = fp_fq[0].second >> 8;
								heavy[i+2] = fp_fq[0].second & 255;
								fp_fq.erase(fp_fq.begin());
								usage4++;
							}
							else {
								for (int j = i; j < num2*2+num3*5/2+num4*3; j++)
									heavy[j] = 0;
								break;
							}
						}
						for (int i = num2*2; i < num2*2+num3*5/2; i += 5) {
							if (!fp_fq.empty()) {
								heavy[i] = fp_fq[0].first.second;
								heavy[i+1] = fp_fq[0].second >> 4;
								heavy[i+2] = (fp_fq[0].second&15) << 4;
								fp_fq.erase(fp_fq.begin());
								usage3++;
							}
							else {
								for (int j = i; j < num2*2+num3*5/2; j++)
									heavy[j] = 0;
								break;
							}
							if (!fp_fq.empty()) {
								heavy[i+4] = fp_fq[0].first.second;
								heavy[i+3] = fp_fq[0].second >> 4;
								heavy[i+2] |= (fp_fq[0].second & 15);
								fp_fq.erase(fp_fq.begin());
								usage3++;
							}
							else {
								for (int j = i+3; j < num2*2+num3*5/2; j++)
									heavy[j] = 0;
								break;
							}
						}
						usage &= 0xffff0000;
						usage += usage2;
						usage += (num3<<4);
						usage += (usage3<<8);
						usage += (num4<<12);
						usage += (usage4<<14);
						return true;
					}					
					int start = num2*2 + num3*5/2;
					int end = start + num4*3;
					if (usage4 < num4) {//exist empty space
						for (int i = start; i < end; i += 3) {
							if (i >= landa_h) printf("Error warning!\n");
							if (heavy[i] == 0) {
								heavy[i] = f;
								heavy[i+1] = fq >> 8;
								heavy[i+2] = fq & 255;
								usage += (1<<14);
								return true;
							}
							std::cout<<"Error warning 4!"<<std::endl;
						}
					}
					break;
				}
			default:break;
		}
		return false; 
	}
	
	bool Exchange(int level, int pos, int fq, uint8_t f)
	{
		double ran = 1.0 * rand() / RAND_MAX;
		switch(level)
		{
			case 2:
				{
					if (ran > c1) return false;
					int num3 = (usage>>4) & 15;
					int num4 = (usage>>12) & 3;
					int num2 = 8 - num4*3/2 - num3*5/4;
					int usage3 = (usage>>8) & 15;
					int minf = -1;
					uint8_t minfp = -1;
					int minfq = -1;
					if (num3 == 0) return false;
					for (int i = num2*2; i < num2*2+num3*5/2; i += 5) {
						int fq = (heavy[i+1]<<4) + ((heavy[i+2]>>4)&15);
						if (minf == -1) {
							minf = i;
							minfp = heavy[i];
							minfq = fq;
						}
						else {
							if (abs(fq) < abs(minfq)) {
								minf = i;
								minfp = heavy[i];
								minfq = fq;
							}
						}
						fq = (heavy[i+3]<<4) + (heavy[i+2]&15);
						if (minf == -1) {
							minf = i + 4;
							minfp = heavy[i+4];
							minfq = fq;
						}
						else {
							if (abs(fq) < abs(minfq)) {
								minf = i + 4;
								minfp = heavy[i+4];
								minfq = fq;
							}
						}
					}
					if ((minfq <= 127) && (minfq >= -128)) {
						if (minf == 6 || minf == 11) {
							heavy[minf] = f;
							heavy[minf+1] = fq >> 4;
							heavy[minf+2] = (heavy[minf+2]&15) + ((fq&15)<<4);
							heavy[pos] = minfp;
							heavy[pos+1] = minfq;
							return true;
						}
						else if (minf == 10 || minf == 15) {
							minf -= 4;
							heavy[minf+4] = f;
							heavy[minf+3] = fq >> 4;
							heavy[minf+2] = (heavy[minf+2]&0xf0) + (fq&15);
							heavy[pos] = minfp;
							heavy[pos+1] = minfq;
							return true;
						}
					}
					break;
				}
			case 3:
				{
					if (ran > c1) return false;
					int num3 = (usage>>4) & 15;
					int num4 = (usage>>12) & 3;
					int num2 = 8 - num4*3/2 - num3*5/4;
					int usage4 = (usage>>14) & 3;
					int minf = -1;
					uint8_t minfp = -1;
					int minfq = -1;
					if (num4 == 0) return false;
					for (int i = num2*2+num3*5/2; i < num2*2+num3*5/2+num4*3; i += 3) {
						int fq = (heavy[i+1]<<8) + (heavy[i+2]);
						if (minf == -1) {
							minf = i;
							minfp = heavy[i];
							minfq = fq;
						}
						else {
							if (abs(fq) < abs(minfq)) {
								minf = i;
								minfp = heavy[i];
								minfq = fq;
							}
						}
					}
					if ((minfq <= 0x7ff) && (minfq >= -0x800)) {
						heavy[minf] = f;
						heavy[minf+1] = fq >> 8;
						heavy[minf+2] = fq & 255;
						if (pos == 0 || pos == 5) {
							heavy[pos] = minfp;
							heavy[pos+1] = minfq >> 4;
							heavy[pos+2] = (heavy[pos+2]&15) + ((minfq&15)<<4);
							return true;
						}
						else if (pos == 4 || pos == 9) {
							pos -= 4;
							heavy[pos+4] = minfp;
							heavy[pos+3] = minfq >> 4;
							heavy[pos+2] = (heavy[pos+2]&0xf0) + (minfq&15);
							return true;
						}
					}
					break;
				}
			default: break;
		}
		return false;
	}
	
	void Insert(int shash, uint8_t f)
	{
		int s[2] = {-1, 1};
		//if exist the flow
		//level 4
		int num3 = (usage>>4) & 15;
		int num4 = (usage>>12) & 3;
		int num2 = 8 - num4*3/2 - num3*5/4;	
		int start = num2*2 + num3*5/2;
		int end = start + num4*3;
		for (int j = start; j < end; j += 3) {
			if (j >= landa_h) printf("Error warning!\n");
			uint8_t e = heavy[j];
			if (e == f) {
				int fq = (heavy[j+1]<<8) + (uint8_t)heavy[j+2] + s[shash];
				heavy[j+1] = fq >> 8;
				heavy[j+2] = fq & 255;
				return;
			}
		}
		//level 3
		start = num2*2;
		end = start + num3*5/2;
		for (int j = start; j < end; j += 5) {
			if (j >= landa_h) printf("Error warning!\n");
			uint8_t e = heavy[j];
			if (e == f) {
				int fq = (heavy[j+1]<<4) + ((heavy[j+2]>>4)&15) + s[shash];
				if ((fq <= 0x7ff) && (fq >= -0x800)) {
					heavy[j+1] = fq >> 4;
					heavy[j+2] = (heavy[j+2]&15) + ((fq&15)<<4);
				}
				else {
					if (Exchange(3, j, fq, f)) return;
					if (Levelup(3, j, fq, f)) return;
					else Delete(3, j, fq, f);
				}
				return;
			}
			e = heavy[j+4];
			if (e == f) {
				int fq = (heavy[j+3]<<4) + (heavy[j+2]&15) + s[shash];
				if ((fq <= 0x7ff) && (fq >= -0x800)) {
					heavy[j+3] = fq >> 4;
					heavy[j+2] = (heavy[j+2]&0xf0) + (fq&15);
				}
				else {
					if (Exchange(3, j+4, fq, f)) return;
					if (Levelup(3, j+4, fq, f)) return;
					else Delete(3, j+4, fq, f);
				}
				return;
			}
		}
		//level 2
		start = 0;
		end = start + num2*2;
		for (int j = start; j < end; j += 2) {
			if (j >= landa_h) printf("Error warning!\n");
			uint8_t e = heavy[j];
			if (e == f) {
				int fq = heavy[j+1] + s[shash];
				if ((fq <= 127) && (fq >= -128))
					heavy[j+1] = fq;
				else {
					if (Exchange(2, j, fq, f)) return;
					if (Levelup(2, j, fq, f)) return;
					else Delete(2, j, fq, f);
				}
				return;
			}
		}
		//no existing flow
		int usage2 = usage & 15;
		if (usage2 < num2) {
			Levelup(1, 0, s[shash], f);
			return;
		}
		//sharing counter
		Waving(s[shash], f);
	}
	
	int Query(int shash, uint8_t f)
	{
		int s[2] = {-1, 1};
		//level 4
		int num3 = (usage>>4) & 15;
		int num4 = (usage>>12) & 3;
		int num2 = 8 - num4*3/2 - num3*5/4;	
		int start = num2*2 + num3*5/2;
		int end = start + num4*3;
		for (int j = start; j < end; j += 3) {
			uint8_t e = heavy[j];
			if (e == f) {
				return ((heavy[j+1]<<8) + (uint8_t)heavy[j+2]) * s[shash];
			}
		}
		//level 3
		start = num2*2;
		end = start + num3*5/2;
		for (int j = start; j < end; j += 5) {
			uint8_t e = heavy[j];
			if (e == f) {
				return ((heavy[j+1]<<4) + ((heavy[j+2]>>4)&15)) * s[shash];
			}
			e = heavy[j+4];
			if (e == f) {
				return ((heavy[j+3]<<4) + (heavy[j+2]&15)) * s[shash];
			}
		}
		//level 2
		start = 0;
		end = start + num2*2;
		for (int j = start; j < end; j += 2) {
			uint8_t e = heavy[j];
			if (e == f) {
				return heavy[j+1] * s[shash];
			}
		}
		//sharing counter
		int count = usage >> 16;
		return count * s[shash];
	}
	
	void output(int hash, uint8_t f)
	{
		int num3 = (usage>>4) & 15;
		int num4 = (usage>>12) & 3;
		int num2 = 8 - num4*3/2 - num3*5/4;
		int start = 0;
		int end = num2*2;
		std::cout<<hash<<std::endl;
		std::cout<<"num2 "<<num2<<std::endl;
		for (int j = start; j < end; j += 2) {
			uint8_t e = heavy[j];
			std::cout<<(int)e<<" "<<(int)heavy[j+1]<<std::endl;
		}
		start = num2*2;
		end = start + num3*5/2;
		std::cout<<"num3 "<<num3<<std::endl;
		for (int j = start; j < end; j += 5) {
			uint8_t e = heavy[j];
			std::cout<<(int)e<<" "<<((int)(heavy[j+1]<<4)+((heavy[j+2]>>4)&15))<<std::endl;
			e = heavy[j+4];
			std::cout<<(int)e<<" "<<((int)(heavy[j+3]<<4)+(heavy[j+2]&15))<<std::endl;
		}
		start = num2*2 + num3*5/2;
		end = start + num4*3;
		std::cout<<"num4 "<<num4<<std::endl;
		for (int j = start; j < end; j += 3) {
			uint8_t e = heavy[j];
			std::cout<<(int)e<<" "<<((int)(heavy[j+1]<<8) + (uint8_t)heavy[j+2])<<std::endl;
		}
	}
};
