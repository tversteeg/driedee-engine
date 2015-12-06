#pragma once

#include "l_vector.h"
#include "l_utils.h"

typedef enum {PORTAL, WALL} edgetype_t;

typedef struct _edge_t edge_t;
typedef struct _sector_t sector_t;

struct _edge_t {
	unsigned int vertex1, vertex2;
	edgetype_t type;
	int sector;
	union {
		edge_t *neighbor;
		struct {
			double uvdiv;
			unsigned char texture;
		};
	};
};

struct _sector_t {
	xy_t *vertices;
	unsigned int nedges;
	edge_t *edges;
	void *lastsprite;

	double floor, ceil;
	unsigned char floortex, ceiltex;
};

sector_t* createSector(xy_t start, edgetype_t type);
void deleteSector(sector_t *sector);

sector_t* getSector(unsigned int index);
unsigned int getNumSectors();
/* Return the index of a sector and -1 if it's not found */
int getIndexSector(const sector_t *sector);

edge_t* createEdge(sector_t *sector, xy_t next, edgetype_t type);

bool pointInSector(const sector_t *sector, xy_t point);

void debugPrintSector(const sector_t *sector, bool verbose);
