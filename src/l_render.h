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

sectp_t createSector(int16_t floor, int16_t ceil);

void addVertToSector(p_t vert, sectp_t nextsect);

void setRenderTarget(texture_t *target);
void renderFromSector(sectp_t sect, p_t camloc, v_t camangle);
