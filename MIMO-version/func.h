#pragma once

#include <iostream>

inline uint32_t fmix(unsigned int hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash;
}

inline unsigned int MurmurHash(const char* str, int len, int seed)
{
	unsigned int h1 = seed;
	unsigned int t1 = 0xcc932d51;
	unsigned int t2 = 0x1b873593;
	for (int i = 0; i < len/4; i++) {
		unsigned int* k0 = (unsigned int*)str;
		unsigned int k1 = k0[i];
		k1 *= t1;
		k1 = (k1<<15) | (k1>>17);
		k1 *= t2;
		h1 ^= k1;
		h1 = (h1<<13) | (h1>>19);
		h1 = h1*5 + 0xe6546b64;
	}
	unsigned int k1 = 0;
	if (len & 3) {
		k1 ^= str[12];
		k1 *= t1;
		k1 = (k1<<15) | (k1>>17);
		k1 *= t2;
		h1 ^= k1;
	}
	h1 ^= len;
	h1 = fmix(h1);
	return h1;
}

inline unsigned int SHash(unsigned int hash)
{
	unsigned int a1 = 378551;
	unsigned int a2 = 63689;
	unsigned int res = 0; 
	for (int i = 0; i < 4; i++) {
		res = res * a2 + ((hash>>(i*8))&0xff);
		a2 = a2 * a1;
	}
	return res;
}

