#include "l_sector.h"

sector_t *sectors = NULL;
unsigned int nsectors = 0;

unsigned int createSector()
{
	sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(sector_t));

	return nsectors;
}
