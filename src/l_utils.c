#include <string.h>
#include <stdlib.h>

#include "l_utils.h"

#ifndef max
#define max(a,b) (a<b?b:a)
#endif

void poolInitialize(pool_t *p, unsigned int elementSize, unsigned int blockSize)
{
	unsigned int i;

	p->elementSize = max(elementSize, sizeof(poolfree_t));
	p->blockSize = blockSize;
	p->used = blockSize - 1;
	p->block = -1;

	for(i = 0; i < POOL_BLOCKS; i++)
		p->blocks[i] = NULL;
	
	p->freed = NULL;
}

void poolFreePool(pool_t *p)
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

void *poolMalloc(pool_t *p)
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
			p->blocks[p->block] = (char*)malloc(p->elementSize * p->blockSize);
		}
	}
	
	return p->blocks[p->block] + p->used * p->elementSize;
}

void poolFree(pool_t *p, void *ptr)
{
	poolfree_t *pFreed = p->freed;
	p->freed = (poolfree_t*)ptr;
	p->freed->nextFree = pFreed;
}

void poolFreeAll(pool_t *p)
{
	p->used = p->blockSize - 1;
	p->block = -1;
	p->freed = NULL;
}
