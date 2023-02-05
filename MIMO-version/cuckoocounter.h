#pragma once

#include <iostream>

#define BUCKET_NUM_CC (HEAVY_MEM / 8)
#define maxloop 2

class hg_node_cc
{
public:
	uint8_t fingerprint[4];
	uint8_t count12;
	uint8_t count3;
	uint16_t count4;
	hg_node_cc()
	{
		for (int i = 0; i < 4; i++)
			fingerprint[i] = 0;
		count12 = 0;
		count3 = 0;
		count4 = 0;
	}
};

vector<vector<hg_node_cc>>hg(range);

bool overflow(int hash, int j, int r)
{
	if (j == 1) {
		if (hg[r][hash].count3 < 0xf) {
			char tmp = hg[r][hash].fingerprint[0];
			hg[r][hash].fingerprint[0] = hg[r][hash].fingerprint[2];
			hg[r][hash].fingerprint[2] = tmp;
			hg[r][hash].count12 = hg[r][hash].count12&0xf0 | hg[r][hash].count3;
			hg[r][hash].count3 = 0xf;
		}
		else if (hg[r][hash].count4 < 0xf) {
			char tmp = hg[r][hash].fingerprint[0];
			hg[r][hash].fingerprint[0] = hg[r][hash].fingerprint[3];
			hg[r][hash].fingerprint[3] = tmp;
			hg[r][hash].count12 = hg[r][hash].count12&0xf0 | (uint8_t)hg[r][hash].count4;
			hg[r][hash].count4 = 0xf;
		}
		else return false;
	}
	else if (j == 2) {
		if (hg[r][hash].count3 < 0xf) {
			char tmp = hg[r][hash].fingerprint[1];
			hg[r][hash].fingerprint[1] = hg[r][hash].fingerprint[2];
			hg[r][hash].fingerprint[2] = tmp;
			hg[r][hash].count12 = hg[r][hash].count12&0xf | (hg[r][hash].count3<<4);
			hg[r][hash].count3 = 0xf;
		}
		else if (hg[r][hash].count4 < 0xf) {
			char tmp = hg[r][hash].fingerprint[1];
			hg[r][hash].fingerprint[1] = hg[r][hash].fingerprint[3];
			hg[r][hash].fingerprint[3] = tmp;
			hg[r][hash].count12 = hg[r][hash].count12&0xf | (uint8_t)(hg[r][hash].count4<<4);
			hg[r][hash].count4 = 0xf;
		}
		else return false;
	}
	else if (j == 3) {
		if (hg[r][hash].count4 < 0xff) {
			char tmp = hg[r][hash].fingerprint[2];
			hg[r][hash].fingerprint[2] = hg[r][hash].fingerprint[3];
			hg[r][hash].fingerprint[3] = tmp;
			hg[r][hash].count3 = (uint8_t)hg[r][hash].count4;
			hg[r][hash].count4 = 0xff;
		}
		else return false;
	}
	return true;
}

bool addone(int hash, int j, int r)
{
	if (j == 1) hg[r][hash].count12++;
	else if (j == 2) hg[r][hash].count12 += 0x10;
	else if (j == 3) hg[r][hash].count3++;
	else if (j == 4) hg[r][hash].count4++;
	if (((hg[r][hash].count12&0xf)==0xf)&&(j==1) || ((hg[r][hash].count12&0xf0)==0xf0)&&(j==2) || (hg[r][hash].count3==0xff)&&(j==3)) {
		bool res = overflow(hash, j, r);
		if (!res) return false;
	}
	return true;
}

void kickout(int loop, uint8_t count, int hash1, int hash2, int j, int r)
{
	int rehash = hash2;
	if (hg[r][rehash].fingerprint[0] == 0 && count < 0xf) {
		hg[r][rehash].fingerprint[0] = hg[r][hash1].fingerprint[j-1];
		hg[r][rehash].count12 = (hg[r][rehash].count12 & 0xf0) | count;
	}
	else if (hg[r][rehash].fingerprint[1] == 0 && count < 0xf) {
		hg[r][rehash].fingerprint[1] = hg[r][hash1].fingerprint[j-1];
		hg[r][rehash].count12 = (hg[r][rehash].count12 & 0xf) | (count << 4);
	}
	else if (hg[r][rehash].fingerprint[2] == 0 && count < 0xff) {
		hg[r][rehash].fingerprint[2] = hg[r][hash1].fingerprint[j-1];
		hg[r][rehash].count3 = count;
	}
	else if (hg[r][rehash].fingerprint[3] == 0 && count < 0xffff) {
		hg[r][rehash].fingerprint[3] = hg[r][hash1].fingerprint[j-1];
		hg[r][rehash].count4 = (uint16_t)count;
	}
	else if (loop > 0) {
		loop--;
		if (count < 0xf) {
			uint8_t tmpcount = hg[r][rehash].count12 & 0xf;
			kickout(loop, tmpcount, rehash, hash1, 1, r);
			hg[r][rehash].fingerprint[0] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count12 = (hg[r][rehash].count12&0xf0) | count;					
		}
		else if (count < 0xff) {
			uint8_t tmpcount = hg[r][rehash].count3;
			kickout(loop, tmpcount, rehash, hash1, 3, r);
			hg[r][rehash].fingerprint[2] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count3 = count;
		}
		else {
			uint16_t tmpcount = hg[r][rehash].count4;
			kickout(loop, tmpcount, rehash, hash1, 4, r);
			hg[r][rehash].fingerprint[3] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count4 = (uint16_t)count;
		}
	}
	else {
		if (count < 0xf) {
			hg[r][rehash].fingerprint[1] = hg[r][hash1].fingerprint[j-1];
			if (count > ((hg[r][rehash].count12&0xf0)>>4))
				hg[r][rehash].count12 = (hg[r][rehash].count12&0xf) | (count<<4);
		}
		else if (count < 0xff) {
			hg[r][rehash].fingerprint[2] = hg[r][hash1].fingerprint[j-1];
			if (count > hg[r][rehash].count3)
				hg[r][rehash].count3 = count;
		}
	}
}

void kickoverflow(int hash1, int hash2, int j, int r)
{
	int rehash = hash2;
	if (j == 1 || j == 2) {
		if (hg[r][rehash].fingerprint[2] == 0) {
			hg[r][rehash].fingerprint[2] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count3 = 0xf;
		}
		else if (hg[r][rehash].fingerprint[3] == 0) {
			hg[r][rehash].fingerprint[3] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count4 = 0xf;
		}
		else if (hg[r][rehash].count3 < 0xf) {
			kickout(maxloop, hg[r][rehash].count3, rehash, hash1, 3, r);
			hg[r][rehash].fingerprint[2] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count3 = 0xf;
		}
		else if (hg[r][rehash].count4 < 0xf) {
			kickout(maxloop, hg[r][rehash].count4, rehash, hash1, 4, r);
			hg[r][rehash].fingerprint[3] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count4 = 0xf;
		}
		else return;
		if (j == 1) hg[r][hash1].count12 &= 0xf0;
		else hg[r][hash1].count12 &= 0xf;
	}
	else if (j == 3) {
		if (hg[r][rehash].fingerprint[3] == 0) {
			hg[r][rehash].fingerprint[3] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count4 = 0xff;
		}
		else if (hg[r][rehash].count4 < 0xff) {
			kickout(maxloop, hg[r][rehash].count4, rehash, hash1, 4, r);
			hg[r][rehash].fingerprint[3] = hg[r][hash1].fingerprint[j-1];
			hg[r][rehash].count4 = 0xff;
		}
		else return;
		hg[r][hash1].count3 = 0;
	}
	hg[r][hash1].fingerprint[j-1] = 0;
}

void Insert(int hash1, int hash2, uint8_t fp, int r)
{
	int hash[2] = {hash1, hash2};
	int ii, jj, flag = 0;
	for (int i = 0; i < 2; i++) {
		for(int j = 1; j <= 4; j++) {
			if (hg[r][hash[i]].fingerprint[j-1] == fp) {
				bool res = addone(hash[i], j, r);
				if (!res) kickoverflow(hash[i], hash[1-i], j, r);
				return;
			}
			else if (!flag && hg[r][hash[i]].fingerprint[j-1] == 0) {
				ii = i;
				jj = j;
				flag = 1;
			}
		}
	}
	if (flag) {
		hg[r][hash[ii]].fingerprint[jj-1] = fp;
		addone(hash[ii], jj, r);
		return;
	}
	int i = rand() % 2;
	uint8_t count = hg[r][hash[i]].count12 & 0xf;
	kickout(maxloop, count, hash[i], hash[1-i], 1, r);
	hg[r][hash[i]].fingerprint[0] = fp;
	hg[r][hash[i]].count12 = (hg[r][hash[i]].count12&0xf0) | 0x1;	
}

int query(int hash, int j, int r)
{
	if (j == 1) return (hg[r][hash].count12&0xf);
	else if (j == 2) return ((hg[r][hash].count12&0xf0) >> 4);
	else if (j == 3) return hg[r][hash].count3;
	else if (j == 4) return hg[r][hash].count4;
	return 0;
}

int Query(int hash1, int hash2, int fp, int r)
{
	int hash[2] = {hash1, hash2};
	for (int i = 0; i < 2; i++) {
		for (int j = 1; j <= 4; j++) {
			if (hg[r][hash[i]].fingerprint[j-1] == fp)
				return query(hash[i], j, r);
		}
	}
	double min1=std::min(hg[r][hash[0]].count12&0xf, hg[r][hash[1]].count12&0xf);

	double min2=(std::min(hg[r][hash[0]].count12&0xf0, hg[r][hash[1]].count12&0xf0))>>4;

	return std::min(min1, min2);
	//int i = rand() % 2;
	//return (bucket[i][hash[i]].count12&0xf) / (1+(bucket[i][hash[i]].count12&0xf)/k);
}
