#pragma once

#include "l_sector.h"
#include "l_draw.h"

#define DRAW_DEBUG_LEVEL 1

typedef struct {
	xyz_t pos;
	double znear, zfar, fov, angle;
	double anglesin, anglecos;
} camera_t;

void setCameraRotation(camera_t *cam, v_t angle);

void calculateViewport(camera_t *cam, xy_t right);

void initRender(unsigned int width, unsigned int height, camera_t *cam);

void renderFromSector(texture_t *targets, texture_t *texturestack, sector_t *sector, camera_t *cam);
