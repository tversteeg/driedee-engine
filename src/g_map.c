#include "g_map.h"

map_t* createMap(unsigned int width, unsigned int height)
{
	map_t *map = (map_t*)malloc(sizeof(map_t));

	map->size = widt * height;
	map->width = width;
	map->tiles = (tile_t*)malloc(sizeof(tile_t) * map->size);

	return map;
}

void destroyMap(map_t *map)
{
	free(map->tiles);
	free(map);
	map = NULL;
}

void setMapTileFromChar(map_t *map, unsigned int index, char type)
{
	map->tiles[index] = (tile_t)type;
}

unsigned int getMapX(map_t *map, unsigned int index)
{
	return index % map->width;
}

unsigned int getMapY(map_t *map, unsigned int index)
{
	return index / map->width;
}

unsigned int getMapIndex(map_t *map, unsigned int x, unsigned int y)
{
	return x + y * map->width;
}
