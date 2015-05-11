#pragma once

#include "l_sector.h"

typedef struct _sprite_t {
	xyz_t pos;
	xy_t scale;
	char texture;
	struct _sprite_t *next, *prev;
} sprite_t;

bool loadLevel(const char *filename);

sprite_t *spawnSprite(sector_t *sect, xyz_t pos, xy_t scale, char texture);
void destroySprite(sector_t *sect, sprite_t *sprite);

sector_t *tryMoveSprite(sector_t *sect, sprite_t *sprite, xy_t pos);

edge_t *findWallRay(const sector_t *sect, xy_t point, xy_t dir);
