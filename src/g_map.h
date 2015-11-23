#pragma once

typedef char tile_t;

typedef struct {
	tile_t *tiles;
	unsigned int size, width;
} map_t;

map_t* createMap(unsigned int width, unsigned int height);
void destroyMap(map_t *map);

void generateNoiseMap(map_t *map, int seed);

void setMapTileFromChar(map_t *map, unsigned int index, char type);

unsigned int getMapX(map_t *map, unsigned int index);
unsigned int getMapY(map_t *map, unsigned int index);
unsigned int getMapIndex(map_t *map, unsigned int x, unsigned int y);
