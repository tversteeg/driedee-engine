#include "l_sector.h"

#include "g_level.h"

#include <stdio.h>

#define EDGE_LEFT 0
#define EDGE_RIGHT 2
#define EDGE_TOP 1
#define EDGE_BOTTOM 3

void createLevelFromMap(map_t *map)
{
	unsigned int i;
	for(i = 0; i < map->size; i++){
		if(map->tiles[i] != '.'){
			continue;
		}
		xy_t vert = {getMapX(map, i) * 32, getMapY(map, i) * 32};

		sector_t *sect = createSector(vert, WALL);
		vert.x += 32;
		createEdge(sect, vert, WALL)->texture = 1;
		vert.y += 32;
		createEdge(sect, vert, WALL);
		vert.x -= 32;
		createEdge(sect, vert, WALL)->texture = 1;

		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 50;
		sect->floor = -10;

		// Find neighbors to create portals to
		if(i > 0 && i % map->width != 0 && map->tiles[i - 1] == '.'){
			edge_t *edge1 = sect->edges + EDGE_LEFT;
			sector_t *sect2 = getSector(getNumSectors() - 2);
			edge_t *edge2 = sect2->edges + EDGE_RIGHT;

			edge1->neighbor = edge2;
			edge2->neighbor = edge1;
			edge1->type = PORTAL;
			edge2->type = PORTAL;
		}

		if(i > map->width && map->tiles[i - map->width] == '.'){
			vert = sect->vertices[sect->edges[EDGE_TOP].vertex2];

			unsigned int j;
			for(j = 0; j < getNumSectors() - 1; j++){
				sector_t *sect2 = getSector(j);
				edge_t *edge2 = sect2->edges + EDGE_BOTTOM;
				xy_t vert2 = sect2->vertices[edge2->vertex1];
				if(vert2.x == vert.x && vert2.y == vert.y){
					edge_t *edge1 = sect->edges + EDGE_TOP;

					edge1->type = PORTAL;
					edge2->type = PORTAL;
					edge1->neighbor = edge2;
					edge2->neighbor = edge1;
					break;
				}
			}
		}
	}
}
