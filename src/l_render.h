#pragma once

#include <stdint.h>

#include <ccVector/ccVector.h>

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
	v_t pitch, yaw;
	sectp_t sect;

	ccMat4x4 persm, modelm;
} camera_t;

sectp_t createSector(int16_t floor, int16_t ceil);

void addVertToSector(p_t vert, sectp_t nextsect);

sectp_t getSector(p_t xz, int32_t y);

void setRenderTarget(texture_t *target);
void renderFromSector(camera_t cam);

void createPerspProjMatrix(camera_t *cam, v_t fov, v_t aspect, v_t znear, v_t zfar);

void moveCamera(camera_t *cam, p_t xz, int32_t y);
void rotateCamera(camera_t *cam, v_t angle);
