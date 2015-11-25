#include "l_sector.h"

#include "g_level.h"

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
	unsigned int i;
	for(i = 0; i < map->size; i++){
		if(map->tiles[i] != '.'){
			continue;
		}
		xy_t vert = {getMapX(map, i) * 32, getMapY(map, i) * 32};
		edge_t edge;
		edge.type = getEdgeType(map, i, 0, -1);
		sector_t *sect = createSector(vert, &edge);
		vert.x += 32;
		edge.type = getEdgeType(map, i, 1, 0);
		createEdge(sect, vert, &edge);
		vert.y += 32;
		edge.type = getEdgeType(map, i, 0, 1);
		createEdge(sect, vert, &edge);
		vert.x -= 32;
		edge.type = getEdgeType(map, i, -1, 0);
		createEdge(sect, vert, &edge);

		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 10;
		sect->floor = -10;
	}
}
