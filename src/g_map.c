#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "g_map.h"

map_t* createMap(unsigned int width, unsigned int height)
{
	map_t *map = (map_t*)malloc(sizeof(map_t));

	map->size = width * height;
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

void generateNoiseMap(map_t *map, int seed)
{
	time_t *tseed = (time_t*)NULL;
	if(seed != 0){
		*tseed = (time_t)seed;
	}
	srand(time(tseed));

	unsigned int i;
	for(i = 0; i < map->size; i++){
		char tile = '.';
		if(i % map->width == 0 || i % (map->width) == map->width - 1 || i < map->width || i > map->size - map->width){
			tile = '#';
		}else if(rand() % 5 == 0){
			tile = '#';
		}
		setMapTileFromChar(map, i, tile);
	}
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

void debugPrintMap(map_t *map)
{
	putc('\n', stdout);

	unsigned int i;
	for(i = 0; i < map->size; i++){
		putc(map->tiles[i], stdout);
		if(i % map->width == map->width - 1){
			putc('\n', stdout);
		}
	}
}
