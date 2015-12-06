#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "l_sector.h"

static sector_t *sectors = NULL;
static unsigned int nsectors = 0;

static sector_t* mallocSector()
{
	if(nsectors == 0){
		nsectors++;
		sectors = (sector_t*)calloc(1, sizeof(sector_t));
	}else{
		sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(sector_t));
		memset(sectors + nsectors - 1, 0, sizeof(sector_t));
	}

	return sectors + nsectors - 1;
}

sector_t* createSector(xy_t start, edgetype_t type)
{
	sector_t *sector = mallocSector();

	sector->edges = (edge_t*)calloc(1, sizeof(edge_t));
	sector->nedges = 1;

	edge_t *edge2 = sector->edges;
	edge2->type = type;
	edge2->sector = nsectors - 1;

	sector->vertices = (xy_t*)calloc(1, sizeof(xy_t));
	sector->vertices[0] = start;

	sector->ceil = 10;
	sector->floor = -10;

	return sector;
}

void deleteSector(sector_t *sector)
{
	fprintf(stderr, "Not implemented deleteSector\n");
	exit(1);
	//TODO reimplement
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

int getIndexSector(const sector_t *sector)
{
	unsigned int i;
	for(i = 0; i < nsectors; i++){
		if(sectors + i == sector){
			return i;
		}
	}	
	
	return -1;
}

edge_t *createEdge(sector_t *sector, xy_t next, edgetype_t type)
{	
	sector->vertices = (xy_t*)realloc(sector->vertices, ++sector->nedges * sizeof(xy_t));
	sector->vertices[sector->nedges - 1] = next;

	sector->edges = (edge_t*)realloc(sector->edges, sector->nedges * sizeof(edge_t));
	edge_t *edge2 = sector->edges + sector->nedges - 1;

	edge2->type = type;
	edge2->vertex1 = sector->nedges - 1;
	edge2->vertex2 = sector->nedges - 2;
	edge2->sector = getIndexSector(sector);
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
	unsigned int i, j;
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

void debugPrintSector(const sector_t *sector, bool verbose)
{
	printf("\nSector %d", getIndexSector(sector));
	if(verbose){
		printf(" (%p)\nCeil: %.f, floor: %.f", sector, sector->ceil, sector->floor);
	}
	printf("\nEdges: %d\n", sector->nedges);

	unsigned int i;
	for(i = 0; i < sector->nedges; i++){
		edge_t *edge = sector->edges + i;
		xy_t v1 = sector->vertices[edge->vertex1];
		xy_t v2 = sector->vertices[edge->vertex2];
		printf("\t %d: (%.f,%.f) - (%.f,%.f)", i, v1.x, v1.y, v2.x, v2.y);
		if(verbose){
			if(edge->type == PORTAL){
				printf("\tPORTAL, neighbor: %d (%p)", edge->neighbor->sector, getSector(edge->neighbor->sector));
			}else{
				printf("\tWALL, uv division: %f", edge->uvdiv);
			}
		}
		printf("\n");
	}
}
