#include "l_sector.h"

#include "l_utils.h"
#include "l_level.h"

static pool_t sectors;
static sector_t *first = NULL;

void sectorInitialize()
{
	poolInitialize(&sectors, sizeof(sector_t), 1024);
}

sector_t* createSector(xy_t start, edge_t *edge)
{
	sector_t *sector = (sector_t*)poolMalloc(&sectors);

	sector->edges = (edge_t*)malloc(sizeof(edge_t));
	edge_t *edge2 = sector->edges;
	edge2->type = edge->type;
	edge2->sector = sector;
	edge2->vertex1 = edge2->vertex2 = 0;
	if(edge2->type == WALL){
		edge2->wallbot = edge->wallbot;
		edge2->walltop = edge->walltop;
		edge2->uvdiv = 0;
	}else{
		edge2->neighbor = NULL;
	}

	sector->vertices = (xy_t*)malloc(sizeof(xy_t));
	sector->vertices[0] = start;
	sector->nedges = 1;

	sector->lastsprite = NULL;

	if(first == NULL){
		first = sector;
	}

	return sector;
}

void deleteSector(sector_t *sector)
{
  sector_t *sect = getFirstSector();
  while(sect != NULL){
    unsigned int i;
    for(i = 0; i < sect->nedges; i++){
      edge_t *edge = sect->edges + i;
      if(edge->type == PORTAL && edge->neighbor != NULL && edge->neighbor->sector == sector){
        edge->neighbor = NULL;
      } 
    }
    sect = getNextSector(sect);
  }

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

edge_t *createEdge(sector_t *sector, xy_t next, edge_t *edge)
{	
	sector->vertices = (xy_t*)realloc(sector->vertices, ++sector->nedges * sizeof(xy_t));
	sector->vertices[sector->nedges - 1] = next;

	sector->edges = (edge_t*)realloc(sector->edges, sector->nedges * sizeof(edge_t));
	edge_t *edge2 = sector->edges + (sector->nedges - 1);
	edge2->type = edge->type;

	edge2->vertex1 = sector->nedges - 1;
	edge2->vertex2 = sector->nedges - 2;
	edge2->sector = sector;
	if(edge2->type == WALL){
		xy_t v1 = sector->vertices[edge2->vertex1];
		xy_t v2 = sector->vertices[edge2->vertex2];
		edge2->uvdiv = vectorDistance(v1, v2);

		edge2->wallbot = edge->wallbot;
		edge2->walltop = edge->walltop;
	}else{
		edge2->neighbor = NULL;
	}

	sector->edges[0].vertex2 = sector->nedges - 1;
	if(sector->edges[0].type == WALL){
		xy_t v1 = sector->vertices[sector->edges[0].vertex1];
		xy_t v2 = sector->vertices[sector->edges[0].vertex2];
		sector->edges[0].uvdiv = vectorDistance(v1, v2);
	}

	return edge2;
}
