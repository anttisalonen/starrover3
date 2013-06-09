#ifndef SR_RANDOM_H
#define SR_RANDOM_H

#include <stdint.h>
#include <limits.h>
#include <assert.h>

typedef unsigned char byte;

/* random functions */

void mysrand(uint32_t s);
uint32_t myrand(void);

inline uint32_t myrandi(int i)
{
	assert(i != 0);
	return myrand() % i;
}

inline int myrandi_uniform(int a, int b)
{
	assert(b > a);
	assert(a != b);
	return (myrand() % (b - a)) + a;
}

inline float myrandf_uniform(float a, float b)
{
	assert(b > a);
	int r = myrand();
	return a + (r / (float)INT_MAX) * (b - a);
}

inline byte myrandbyte(void)
{
	return myrand() & 0xff;
}


#endif

