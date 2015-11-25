#include "l_sector.h"

#include "g_level.h"

void createLevelFromMap(map_t *map)
{
	unsigned int i;
	for(i = 0; i < map->size; i++){
		xy_t vert = {getMapX(map, i) * 32, getMapY(map, i) * 32};
		edge_t edge = {.type = 1};
		sector_t *sect = createSector(vert, &edge);
		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 10;
		sect->floor = -10;

		vert.x += 32;
		createEdge(sect, vert, &edge);
		vert.y += 32;
		createEdge(sect, vert, &edge);
		vert.x -= 32;
		createEdge(sect, vert, &edge);
	}
}
