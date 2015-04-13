/* jobtalle's dynamic pool implementation */

#pragma once

#include <ccore/types.h>

#include "l_draw.h"

#define POOL_BLOCKS  1024

typedef struct poolfree_t{
	struct poolfree_t *nextFree;
} poolfree_t;

typedef struct {
	unsigned int elementSize, blockSize, used;
	int block;
	poolfree_t *freed;
	char *blocks[POOL_BLOCKS];
} pool_t;

void poolInitialize(pool_t *p, unsigned int elementSize, unsigned int blockSize);
void poolFreePool(pool_t *p);

void *poolMalloc(pool_t *p);
bool poolIsFree(pool_t *p, void *ptr);
void poolFree(pool_t *p, void *ptr);
void poolFreeAll(pool_t *p);

void *poolGetNext(pool_t *p, void *ptr);

unsigned long hash(const char *string);

pixel_t strtopixel(const char *hexstr);
