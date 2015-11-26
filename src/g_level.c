#include "l_sector.h"

#include "g_level.h"

#include <stdio.h>

void createLevelFromMap(map_t *map)
{
	unsigned int i;
	sector_t *lastsect = NULL;
	for(i = 0; i < map->size; i++){
		if(map->tiles[i] != '.'){
			continue;
		}
		xy_t vert = {getMapX(map, i) * 32, getMapY(map, i) * 32};
		edge_t edge = {.type = 1};
		// Top
		sector_t *sect = createSector(vert, &edge);
		// Right
		vert.x += 32;
		createEdge(sect, vert, &edge)->texture = 1;
		// Bottom
		vert.y += 32;
		createEdge(sect, vert, &edge)->texture = 0;
		// Left
		vert.x -= 32;
		createEdge(sect, vert, &edge)->texture = 0;
		if(i > 0 && i % map->width != 0 && map->tiles[i - 1] == '.'){
			edge_t *edge1 = sect->edges + 3;
			edge_t *edge2 = lastsect->edges + 1;

			edge1->type = edge2->type = 0;
			edge1->neighbor = edge2;
			edge2->neighbor = edge1;
		}

		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 10;
		sect->floor = -10;

		lastsect = sect;
	}
}
