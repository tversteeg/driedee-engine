#include <string.h>
#include <stdlib.h>

#include "l_utils.h"
#include "l_colors.h"

#ifndef max
#define max(a,b) (a<b?b:a)
#endif

#include <stdio.h>

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
		if(p->blocks[++p->block] == NULL) {
			p->blocks[p->block] = (char*)malloc(p->blockSize * p->elementSize);
		}
	}
	
	return p->blocks[p->block] + p->used * p->elementSize;
}

bool poolIsFree(pool_t *p, void *ptr)
{
	if(ptr == NULL){
		return true;
	}else if(p->freed == NULL){
		return false;
	}

	poolfree_t *freed = p->freed;
	while(freed->nextFree != NULL){
		freed = freed->nextFree;
		if(freed == ptr){
			return true;
		}
	}

	return false;
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

void *poolGetNext(pool_t *p, void *ptr)
{
	unsigned int i, j;
	for(i = 0; i <= p->block; i++){
		for(j = 0; j < p->blockSize; j += p->elementSize){
			if(p->blocks[i] + j == ptr){
				goto found;
			}
		}
	}
	return NULL;

found: ;
	void *next = NULL;
	unsigned int totalblocks = p->block * p->blockSize + (p->used + 1) * p->elementSize;
	do{
		if(!poolIsFree(p, next)){
			return next;
		}
		if((j += p->elementSize) == p->blockSize){
			j = 0;
			i++;
			if(i == p->block){
				return NULL;
			}
		}
		next = p->blocks[i] + j;
	}	while(next != NULL && i * p->blockSize + j < totalblocks);

	return NULL;
}

// http://www.cse.yorku.ca/~oz/hash.html
hash_t hash(const char *string)
{
	hash_t val = 538l;

	int c;
	while((c = *string++)){
		val = ((val << 5) + val) + c;
	}

	return val;
}
