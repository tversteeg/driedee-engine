#pragma once

#include "l_sector.h"
#include "l_draw.h"

typedef struct {
	xyz_t pos;
	double znear, zfar, fov, angle;
} camera_t;

void calculateViewport(camera_t *cam, xy_t right);

void renderFromSector(texture_t *texture, texture_t *wall, sector_t *sector, camera_t *cam);
