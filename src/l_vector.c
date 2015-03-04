#include "l_vector.h"

#define V_ERROR 0.0001

#include <math.h>

xy_t vectorUnit(xy_t p)
{
	v_t len = sqrt(p.x * p.x + p.y * p.y);
	p.x /= len;
	p.y /= len;
	return p;
}

bool vectorIsEqual(xy_t p1, xy_t p2)
{
	return p1.x - V_ERROR < p2.x && p1.x + V_ERROR > p2.x && p1.y - V_ERROR < p2.y && p1.y + V_ERROR > p2.y;
}

bool vectorIsBetween(xy_t p, xy_t left, xy_t right)
{
	v_t leftRight = vectorDotProduct(left, right);

	return leftRight < vectorDotProduct(left, p) && leftRight < vectorDotProduct(right, p);
}

bool vectorIsLeft(xy_t p, xy_t s1, xy_t s2)
{
	return (s2.x - s1.x) * (p.y - s1.y) > (s2.y - s1.y) * (p.x - s1.x);
}

v_t vectorDotProduct(xy_t p1, xy_t p2)
{
	return p1.x * p2.x + p1.y * p2.y;
}

v_t vectorCrossProduct(xy_t p1, xy_t p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

v_t vectorProjectScalar(xy_t p1, xy_t p2)
{
	return vectorDotProduct(p1, vectorUnit(p2));
}

xy_t vectorProject(xy_t p1, xy_t p2)
{
	xy_t normal = vectorUnit(p2);
	v_t scalar = vectorDotProduct(p1, normal);

	normal.x *= scalar;
	normal.y *= scalar;

	return normal;
}
