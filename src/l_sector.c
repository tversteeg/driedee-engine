#include "l_sector.h"
#include "l_utils.h"

static pool_t sectors;
static sector_t *first = NULL;

void sectorInitialize()
{
	poolInitialize(&sectors, sizeof(sector_t), 1024);
}

sector_t* createSector(xy_t start, edgetype_t type)
{
	sector_t *sector = (sector_t*)poolMalloc(&sectors);
	sector->edges = (edge_t*)malloc(sizeof(edge_t));
	sector->edges[0] = (edge_t){0, 0, type, sector, .neighbor = NULL};
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

sector_t* getSector(unsigned int index)
{
	sector_t *sector = getFirstSector();
	while(index > 0){
		sector = getNextSector(sector);
		index--;
	}
	return sector;
}

/* TODO speed up this function by using pointer arithmetic */
int getIndexSector(sector_t *sector)
{
	int index = 0;
	sector_t *search = getFirstSector();
	while(search != sector){
		search = getNextSector(search);
		if(search == NULL){
			return -1;
		}
		index++;
	}

	return index;
}

edge_t *createEdge(sector_t *sector, xy_t next, edgetype_t type)
{	
	sector->vertices = (xy_t*)realloc(sector->vertices, ++sector->nedges * sizeof(xy_t));
	sector->vertices[sector->nedges - 1] = next;

	sector->edges = (edge_t*)realloc(sector->edges, sector->nedges * sizeof(edge_t));
	edge_t *edge = sector->edges + (sector->nedges - 1);
	edge->vertex1 = sector->nedges - 1;
	edge->vertex2 = sector->nedges - 2;
	edge->type = type;
	edge->sector = sector;
	edge->neighbor = NULL;
	if(type == WALL){
		/*
		xy_t v1 = sector->vertices[edge->vertex1];
		xy_t v2 = sector->vertices[edge->vertex2];
		edge->uvdiv = vectorDistance(v1, v2);
		*/
		edge->uvdiv = 25;
	}

	sector->edges[0].vertex2 = sector->nedges - 1;
	if(sector->edges[0].type == WALL){
		/*
		xy_t v1 = sector->vertices[sector->edges[0].vertex1];
		xy_t v2 = sector->vertices[sector->edges[0].vertex2];
		sector->edges[0].uvdiv = vectorDistance(v1, v2);
		*/
		edge->uvdiv = 25;
	}

	return edge;
}
