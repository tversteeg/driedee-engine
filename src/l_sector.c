#include "l_sector.h"

static pool *sectors = NULL;

void sectorInitialize()
{

}

sector_t* createSector()
{
	sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(sector_t));

	return sectors + (nsectors - 1);
}


