#pragma once

#include "l_sector.h"

typedef struct {
	sector_t *sect;
	double x, y;
	char texture;
} sprite_t;

bool loadLevel(const char *filename);

sprite_t *createSprite(sector_t *sect, double x, double y, char texture);
void freeSprite(sprite_t *sprite);
