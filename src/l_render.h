#pragma once

#include <stdint.h>

#include "l_vector.h"
#include "l_draw.h"

#define WALL -1

#define MAX_SECTORS 128
typedef int16_t sectp_t;

#define MAX_WALLS 2048
typedef uint16_t wallp_t;

typedef int32_t p_t[2];

typedef struct {
	p_t xz;
	int32_t y;
	v_t angle;
	sectp_t sect;
} camera_t;

sectp_t createSector(int16_t floor, int16_t ceil);

void addVertToSector(p_t vert, sectp_t nextsect);

sectp_t getSector(p_t xz, int32_t y);

void setRenderTarget(texture_t *target);
void renderFromSector(camera_t cam);
