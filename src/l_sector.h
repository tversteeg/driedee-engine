#pragma once

#include "l_vector.h"

typedef enum {PORTAL, WALL} edgetype_t;

typedef struct {
	unsigned int vertex1, vertex2;
	edgetype_t type;
	union {
		unsigned int neighbor;
	};
} edge_t;

typedef struct {
	xy_t *vertices;
	unsigned int nvertices;
	edge_t *walls;
} sector_t;

sector_t* createSector();
