#pragma once

#include <ccore/types.h>

typedef double v_t;

typedef struct {
	double x, y;
} xy_t;

typedef struct {
	double x, y, z;
} xyz_t;

xy_t vectorUnit(xy_t p);

bool vectorIsEqual(xy_t p1, xy_t p2);
bool vectorIsBetween(xy_t p, xy_t left, xy_t right);
bool vectorIsLeft(xy_t p, xy_t s1, xy_t s2);

v_t vectorDotProduct(xy_t p1, xy_t p2);
v_t vectorCrossProduct(xy_t p1, xy_t p2);
v_t vectorProjectScalar(xy_t p1, xy_t p2);

xy_t vectorProject(xy_t p1, xy_t p2);
