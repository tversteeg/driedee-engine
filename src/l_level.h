#pragma once

#include "l_sector.h"

typedef struct _sprite_t {
	double x, y;
	char texture;
	struct _sprite_t *next, *prev;
} sprite_t;

bool loadLevel(const char *filename);

sprite_t *spawnSprite(sector_t *sect, double x, double y, char texture);
void destroySprite(sector_t *sect, sprite_t *sprite);
