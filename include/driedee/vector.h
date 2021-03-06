#pragma once

#include <ccore/types.h>

#define V_ERROR 0.0001

typedef float v_t;

typedef struct {
	v_t x, y;
} xy_t;

#define XY_ZERO ((xy_t){0, 0})

typedef struct {
	v_t x, y, z;
} xyz_t;

#define XYZ_ZERO ((xyz_t){0, 0, 0})

xy_t vectorUnit(xy_t p);

bool vectorIsEqual(xy_t p1, xy_t p2);
bool vectorIsBetween(xy_t p, xy_t left, xy_t right);
bool vectorIsLeft(xy_t p, xy_t s1, xy_t s2);

v_t vectorDotProduct(xy_t p1, xy_t p2);
v_t vectorCrossProduct(xy_t p1, xy_t p2);
v_t vectorProjectScalar(xy_t p1, xy_t p2);

xy_t vectorProject(xy_t p1, xy_t p2);

v_t vectorDistance(xy_t p1, xy_t p2);

bool segmentSegmentIntersect(xy_t p1, xy_t p2, xy_t p3, xy_t p4, xy_t *p);
bool lineSegmentIntersect(xy_t line, xy_t dir, xy_t p1, xy_t p2, xy_t *p);
bool raySegmentIntersect(xy_t ray, xy_t dir, xy_t p1, xy_t p2, xy_t *p);
bool segmentCircleIntersect(xy_t p1, xy_t p2, xy_t circle, v_t radius, xy_t *p);
v_t distanceToSegment(xy_t p, xy_t p1, xy_t p2);
