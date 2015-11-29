#include <stdlib.h>
#include <stdio.h>

#include "l_sector.h"

static sector_t *sectors = NULL;
static unsigned int nsectors = 0;

static sector_t* mallocSector()
{
	if(nsectors == 0){
		sectors = (sector_t*)malloc(sizeof(sector_t));
		nsectors++;
	}else{
		sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(sector_t));
	}

	return sectors + nsectors - 1;
}

sector_t* createSector(xy_t start, edge_t *edge)
{
	sector_t *sector = mallocSector();

	sector->edges = (edge_t*)malloc(sizeof(edge_t));
	edge_t *edge2 = sector->edges;
	edge2->type = edge->type;
	edge2->sector = sector;
	edge2->vertex1 = edge2->vertex2 = 0;
	if(edge2->type == WALL){
		edge2->texture = 0;
		edge2->uvdiv = 0;
	}else{
		edge2->neighbor = NULL;
	}

	sector->vertices = (xy_t*)malloc(sizeof(xy_t));
	sector->vertices[0] = start;
	sector->nedges = 1;

	sector->ceil = 10;
	sector->floor = -10;
	sector->ceiltex = sector->floortex = 0;

	sector->lastsprite = NULL;

	return sector;
}

void deleteSector(sector_t *sector)
{
	//TODO reimplement
	/*
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
	*/
}

sector_t* getSector(unsigned int index)
{
	if(index >= nsectors){
		return NULL;
	}
	return sectors + index;
}

unsigned int getNumSectors()
{
	return nsectors;
}

int getIndexSector(sector_t *sector)
{
	int i;
	for(i = 0; i < nsectors; i++){
		if(sectors + i == sector){
			return i;
		}
	}	
	
	return -1;
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
		edge2->texture = 0;
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

/* http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html#Convex%20Polygons */
bool pointInSector(const sector_t *sector, xy_t point)
{
	bool in = false;
	int i, j;
	for(i = 0, j = sector->nedges - 1; i < sector->nedges; j = i++){
		xy_t pi = sector->vertices[i];
		xy_t pj = sector->vertices[j];
		if(((pi.y > point.y) != (pj.y > point.y)) &&
				(point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x)){
			in = !in;
		}
	}

	return in;
}
