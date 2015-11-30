#include "l_sector.h"

#include "g_level.h"

#include <stdio.h>

#define EDGE_TOP 0
#define EDGE_RIGHT 1
#define EDGE_BOTTOM 2
#define EDGE_LEFT 3

void createLevelFromMap(map_t *map)
{
	unsigned int i;
	for(i = 0; i < map->size; i++){
		if(map->tiles[i] != '.'){
			continue;
		}
		xy_t vert = {getMapX(map, i) * 32, getMapY(map, i) * 32};
		edge_t edge = {.type = WALL};
		// Left
		sector_t *sect = createSector(vert, &edge);
		// Top
		vert.x += 32;
		createEdge(sect, vert, &edge)->texture = 1;
		// Right
		vert.y += 32;
		createEdge(sect, vert, &edge)->texture = 0;
		// Bottom
		vert.x -= 32;
		createEdge(sect, vert, &edge)->texture = 0;

		if(i > 0 && i % map->width != 0 && map->tiles[i - 1] == '.'){
			edge_t *edge1 = sect->edges + EDGE_LEFT;
			sector_t *sect2 = getSector(getNumSectors() - 2);
			edge_t *edge2 = sect2->edges + EDGE_RIGHT;

			edge1->type = PORTAL;
			edge2->type = PORTAL;
			edge1->neighbor = edge2;
			edge2->neighbor = edge1;
		}
		
		vert = sect->vertices[sect->edges[EDGE_TOP].vertex1];
		if(i > map->width && map->tiles[i - map->width] == '.'){
			int j;
			for(j = 0; j < getNumSectors() - 1; j++){
				sector_t *sect2 = getSector(j);
				edge_t *edge2 = sect2->edges + EDGE_BOTTOM;
				xy_t vert2 = sect2->vertices[edge2->vertex1];
				if(vert2.x == vert.x && vert2.y == vert.y){
					edge_t *edge1 = sect->edges;

					edge1->type = PORTAL;
					edge2->type = PORTAL;
					edge1->neighbor = edge2;
					edge2->neighbor = edge1;
					break;
				}
			}
		}

		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 10;
		sect->floor = -10;
	}
}
