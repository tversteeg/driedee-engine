#pragma once

#include "l_vector.h"

typedef enum {PORTAL, WALL} edgetype_t;

typedef struct _edge_t edge_t;
typedef struct _sector_t sector_t;

struct _edge_t {
	unsigned int vertex1, vertex2;
	edgetype_t type;
	sector_t *sector;
	union {
		edge_t *neighbor;
		struct {
			double uvdiv, walltop, wallbot;
			unsigned char texture;
		};
	};
};

struct _sector_t {
	xy_t *vertices;
	unsigned int nedges;
	edge_t *edges;
};

void sectorInitialize();

sector_t* createSector(xy_t start, edge_t *edge);
void deleteSector(sector_t *sector);

sector_t* getFirstSector();
sector_t* getNextSector(sector_t *sector);
sector_t* getSector(unsigned int index);
/* Return the index of a sector and -1 if it's not found */
int getIndexSector(sector_t *sector);

edge_t* createEdge(sector_t *sector, xy_t next, edge_t *edge);
