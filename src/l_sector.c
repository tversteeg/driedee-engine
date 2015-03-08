#include "l_sector.h"

static sector_t *sectors = NULL;
static unsigned int nsectors = 0;
static edge_t *edges = NULL;
static unsigned int nedges = 0;
static xy_t *vertices = NULL;
static unsigned int nvertices = 0;

sector_t* createSector()
{
	sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(sector_t));

	return sectors + (nsectors - 1);
}


