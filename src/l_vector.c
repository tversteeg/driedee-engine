#include "l_vector.h"

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

double vectorDistance(xy_t p1, xy_t p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx * dx + dy * dy);
}

bool segmentSegmentIntersect(xy_t p1, xy_t p2, xy_t p3, xy_t p4, xy_t *p)
{
	double denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
	double n1 = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
	double n2 = (p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x);
	if(fabs(n1) < V_ERROR && fabs(n2) < V_ERROR && fabs(denom) < V_ERROR){
		p->x = (p1.x + p2.x) / 2;
		p->y = (p1.y + p2.y) / 2;
		return true;
	}
	if(fabs(denom) < V_ERROR){
		return false;
	}
	n1 /= denom;
	n2 /= denom;
	if(n1 < -V_ERROR || n1 > 1.001f || n2 < -V_ERROR || n2 > 1.001f){
		return false;
	}
	p->x = p1.x + n1 * (p2.x - p1.x);
	p->y = p1.y + n1 * (p2.y - p1.y);
	return true;
}

bool lineSegmentIntersect(xy_t line, xy_t dir, xy_t p1, xy_t p2, xy_t *p)
{
	xy_t s;
	s.x = p2.x - p1.x;
	s.y = p2.y - p1.y;

	xy_t diff;
	diff.x = p1.x - line.x;
	diff.y = p1.y - line.y;

	double denom = vectorCrossProduct(dir, s);
	double u = vectorCrossProduct(diff, dir);
	if(fabs(denom) < V_ERROR){
		if(fabs(u) < V_ERROR){
			p->x = (line.x + p1.x) / 2;
			p->y = (line.y + p1.y) / 2;
			return true;
		}else{
			return false;
		}
	}
	u /= denom;
	if(u < -V_ERROR || u > 1.0 + V_ERROR){
		return false;
	}
	p->x = p1.x + u * s.x;
	p->y = p1.y + u * s.y;
	return true;
}

bool segmentCircleIntersect(xy_t p1, xy_t p2, xy_t circle, double radius, xy_t *p)
{
	xy_t seg = {p2.x - p1.x, p2.y - p1.y};
	xy_t cir = {circle.x - p1.x, circle.y - p1.y};
	double proj = vectorProjectScalar(cir, seg);

	xy_t closest;
	if(proj < 0){
		closest = p1;
	}else if(proj > sqrt(seg.x * seg.x + seg.y * seg.y)){
		closest = p2;
	}else{
		xy_t projv = vectorProject(cir, seg);
		closest = (xy_t){p1.x + projv.x, p1.y + projv.y};
	}

	double dx = circle.x - closest.x;
	double dy = circle.y - closest.y;
	double dist = sqrt(dx * dx + dy * dy);
	if(dist < radius){
		p->x = closest.x;
		p->y = closest.y;
		return true;
	}else{
		return false;
	}
}

double distanceToSegment(xy_t p, xy_t p1, xy_t p2)
{
	xy_t seg = {p2.x - p1.x, p2.y - p1.y};
	xy_t cir = {p.x - p1.x, p.y - p1.y};
	double proj = vectorProjectScalar(cir, seg);

	xy_t closest;
	if(proj < 0){
		closest = p1;
	}else if(proj > sqrt(seg.x * seg.x + seg.y * seg.y)){
		closest = p2;
	}else{
		xy_t projv = vectorProject(cir, seg);
		closest = (xy_t){p1.x + projv.x, p1.y + projv.y};
	}

	double dx = p.x - closest.x;
	double dy = p.y - closest.y;

	return sqrt(dx * dx + dy * dy);
}
