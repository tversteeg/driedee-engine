#include <string.h>
#include <stdlib.h>

#include "l_pool.h"

#ifndef max
#define max(a,b) (a<b?b:a)
#endif

void poolInitialize(pool *p, unsigned int elementSize, unsigned int blockSize)
{
	unsigned int i;

	p->elementSize = max(elementSize, sizeof(poolFreed));
	p->blockSize = blockSize;
	p->used = blockSize - 1;
	p->block = -1;

	for(i = 0; i < POOL_BLOCKS; i++)
		p->blocks[i] = NULL;
	
	p->freed = NULL;
}

void poolFreePool(pool *p)
{
	unsigned int i;
	for(i = 0; i < POOL_BLOCKS; i++) {
		if(p->blocks[i] == NULL) {
			break;
		}
		else {
			free(p->blocks[i]);
		}
	}
}

void *poolMalloc(pool *p)
{
	if(p->freed != NULL) {
		void *recycle = p->freed;
		p->freed = p->freed->nextFree;
		return recycle;
	}

	p->used++;
	if(p->used == p->blockSize) {
		p->used = 0;
		p->block++;
		if(p->blocks[p->block] == NULL) {
			p->blocks[p->block] = malloc(p->elementSize * p->blockSize);
		}
	}
	
	return p->blocks[p->block] + p->used * p->elementSize;
}

void poolFree(pool *p, void *ptr)
{
	poolFreed *pFreed = p->freed;
	p->freed = ptr;
	p->freed->nextFree = pFreed;
}

void poolFreeAll(pool *p)
{
	p->used = p->blockSize - 1;
	p->block = -1;
	p->freed = NULL;
}
