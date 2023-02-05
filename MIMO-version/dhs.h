#pragma once

#include <iostream>
#include <string.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include "mimosketch.h"

#define BUCKET_NUM_DHS (HEAVY_MEM / 16)
#define b 1.08

class hg_node_dhs
{
public:
	std::vector<unsigned char>heavy;
	unsigned int usage;//num-/used-
	hg_node_dhs()
	{
		heavy = std::vector<unsigned char>(landa_h, 0);
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
						if (ran < std::pow(b, min_fq * -1)) {
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
							std::vector<int> weaks(rest, -1);
							std::vector<int> widx(rest, -1);
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
						std::cout<<"error warning 3!"<<std::endl;
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
							std::vector<int> weaks(rest, -1);
							std::vector<int> widx(rest, -1);
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
							std::cout<<"error warning 4!"<<std::endl;
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
	void Insert(unsigned short f)
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
	int Query(unsigned short f)
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
