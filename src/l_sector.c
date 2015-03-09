#include "l_sector.h"
#include "l_utils.h"

static pool_t sectors;
static sector_t *first = NULL;

void sectorInitialize()
{
	poolInitialize(&sectors, sizeof(sector_t), 1024);
}

sector_t* createSector(xy_t start)
{
	sector_t *sector = (sector_t*)poolMalloc(&sectors);
	sector->edges = (edge_t*)malloc(sizeof(edge_t));
	sector->edges[0] = (edge_t){0, 0, WALL};
	sector->vertices = (xy_t*)malloc(sizeof(xy_t));
	sector->vertices[0] = start;
	sector->nedges = 1;

	if(first == NULL){
		first = sector;
	}

	return sector;
}

void deleteSector(sector_t *sector)
{
	poolFree(&sectors, sector);
}

sector_t* getFirstSector()
{
	return first;
}

sector_t* getNextSector(sector_t *sector)
{
	return (sector_t*)poolGetNext(&sectors, sector);
}

unsigned int createEdge(sector_t *sector, xy_t next, edgetype_t type)
{
	if(vectorIsEqual(next, sector->vertices[0])){
		//TODO handle when a vertex is repeatedly clicked
		return 0;
	}

	sector->vertices = (xy_t*)realloc(sector->vertices, ++sector->nedges * sizeof(xy_t));
	sector->vertices[sector->nedges - 1] = next;

	sector->edges = (edge_t*)realloc(sector->edges, sector->nedges * sizeof(edge_t));
	sector->edges[sector->nedges - 1] = (edge_t){sector->nedges - 1, sector->nedges - 2, type, sector};

	sector->edges[0].vertex2 = sector->nedges - 1;

	return sector->nedges - 1;
}
