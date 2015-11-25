#include "l_sector.h"

#include "g_level.h"

#include <stdio.h>

int getEdgeType(map_t *map, unsigned int index, int offsetx, int offsety){
	int x = getMapX(map, index);
	int nextx = x + offsetx;
	if(nextx < 0 || nextx > map->width){
		return 1;
	}
	int y = getMapY(map, index);
	int nexty = y + offsety;
	if(nexty < 0 || nexty > map->size / map->width){
		return 1;
	}

	if(map->tiles[getMapIndex(map, nextx, nexty)] == '.'){
		return 0;
	}

	return 1;
}

void createLevelFromMap(map_t *map)
{
	unsigned int i, sectors = 0;
	for(i = 0; i < map->size; i++){
		if(map->tiles[i] != '.'){
			continue;
		}
		xy_t vert = {getMapX(map, i) * 32, getMapY(map, i) * 32};
		edge_t edge;
		// Top
		edge.type = getEdgeType(map, i, 0, -1);
		sector_t *sect = createSector(vert, &edge);
		// Right
		vert.x += 32;
		edge.type = getEdgeType(map, i, 1, 0);
		createEdge(sect, vert, &edge);
		// Bottom
		vert.y += 32;
		edge.type = getEdgeType(map, i, 0, 1);
		createEdge(sect, vert, &edge);
		// Left
		vert.x -= 32;
		edge.type = getEdgeType(map, i, -1, 0);
		edge_t *edge1 = createEdge(sect, vert, &edge);
		if(edge.type == 0){
			printf("S: %d i: %d\n", sectors, i);
			edge_t *edge2 = getSector(sectors)->edges + 1;
			edge1->neighbor = edge2;
			edge2->neighbor = edge1;
		}

		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 10;
		sect->floor = -10;

		sectors++;
	}
}
