#include <stdlib.h>
#include <assert.h>

#include "random.h"

void mysrand(uint32_t s)
{
	srand(s);
}

uint32_t myrand(void)
{
	return rand();
}

