#include "l_sector.h"

#include "g_level.h"

void createLevelFromMap(map_t *map)
{
	unsigned int i;
	for(i = 0; i < map->size; i++){
		xy_t vert = {getMapX(map, i), getMapY(map, i)};
		edge_t edge = {.type = 0};
		sector_t *sect = createSector(vert, &edge);

		vert.x = 1;
		createEdge(sect, vert, &edge);
		vert.y = 1;
		createEdge(sect, vert, &edge);
		vert.x = 0;
		createEdge(sect, vert, &edge);
	}
}
