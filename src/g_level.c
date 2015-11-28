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
			edge_t *edge1 = sect->edges;
			edge_t *edge2 = lastsect->edges + 2;

			edge1->type = PORTAL;
			edge2->type = PORTAL;
			edge1->neighbor = edge2;
			edge2->neighbor = edge1;
		}
		
		vert.y -= 32;
		if(i > map->width && map->tiles[i - map->width] == '.'){
			sector_t *sect2 = getFirstSector();
			unsigned int max = i - 1;
			while(max-- && sect2 != NULL){
				edge_t *edge2 = sect2->edges + 3;
				xy_t vert2 = sect2->vertices[edge2->vertex1];
				if(vert2.x == vert.x && vert2.y == vert.y){
					edge_t *edge1 = sect->edges + 1;
					edge1->type = PORTAL;
					edge2->type = PORTAL;
					edge1->neighbor = edge2;
					edge2->neighbor = edge1;
					break;
				}

				sect2 = getNextSector(sect2);
			}
		}

		sect->ceiltex = 0;
		sect->floortex = 0;
		sect->ceil = 10;
		sect->floor = -10;

		lastsect = sect;
	}
}
