#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/time.h>

#ifdef WINDOWS
#include <gl/GL.h>
#else
#include <GL/glew.h>
#endif

#define FL_ERROR 0.001

#define WIDTH 800
#define HEIGHT 600
#define HWIDTH (WIDTH / 2)
#define HHEIGHT (HEIGHT / 2)

#define ASPECT (WIDTH / (double)HEIGHT)

#define PLAYER_SPEED 0.5f
#define PLAYER_JUMP -1.8f
#define PLAYER_FRICTION 0.8f
#define PLAYER_GRAVITY 0.1f

#define RENDER_MAP
//#define USE_MOUSE

typedef struct {
	unsigned char r, g, b;
} pixelRGB_t;

typedef struct {
	double x, y;
} xy_t;

typedef struct {
	double x, y, z;
} xyz_t;

typedef struct {
	xyz_t start;
	double angle, slope;
} plane_t;

typedef enum {PORTAL, WALL} walltype_t;

typedef struct {
	unsigned int vertex1, vertex2;
	walltype_t type;
	union {
		unsigned int neighbor;
	};
} wall_t;

typedef struct {
	xy_t *vertex;
	wall_t *walls;
	plane_t floor, ceil;
	unsigned int npoints, nneighbors, *neighbors, nvisited, *visited;
} sector_t;

struct player {
	xyz_t pos, vel;
	double angle, fov, yaw, height, radius;
	unsigned int sector;
} player;

GLuint texture;
pixelRGB_t pixels[WIDTH * HEIGHT];
sector_t *sectors = NULL;
unsigned int nsectors = 0;

double yLookup[HHEIGHT];

void vline(int x, int top, int bot, int r, int g, int b, double a)
{
	if(x < 0 || x >= WIDTH){
		return;
	}

	if(top < bot){
		int tmp = top;
		top = bot;
		bot = tmp;
	}

	if(top >= HEIGHT){
		top = HEIGHT - 1;
	}
	if(bot < 0){
		bot = 0;
	}

	int y;
	for(y = bot; y <= top; y++){
		pixelRGB_t *pixel = &pixels[x + y * WIDTH];
		if(a == 1){
			pixel->r = r;
			pixel->g = g;
			pixel->b = b;
		}else{
			double mina = 1 - a;
			pixel->r = pixel->r * mina + r * a;
			pixel->g = pixel->g * mina + g * a;
			pixel->b = pixel->b * mina + b * a;
		}
	}
}

void hline(int y, int left, int right, int r, int g, int b, double a)
{
	if(y < 0 || y >= HEIGHT){
		return;
	}

	if(right < left){
		int tmp = right;
		right = left;
		left = tmp;
	}

	if(right >= WIDTH){
		right = WIDTH - 1;
	}
	if(left < 0){
		left = 0;
	}

	int x;
	for(x = left; x <= right; x++){
		pixelRGB_t *pixel = &pixels[x + y * WIDTH];
		if(a == 1){
			pixel->r = r;
			pixel->g = g;
			pixel->b = b;
		}else{
			double mina = 1 - a;
			pixel->r = pixel->r * mina + r * a;
			pixel->g = pixel->g * mina + g * a;
			pixel->b = pixel->b * mina + b * a;
		}
	}
}

void drawPixel(int x, int y, int r, int g, int b, double a)
{
	if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT){
		pixelRGB_t *pixel = &pixels[x + y * WIDTH];
			if(a == 1){
				pixel->r = r;
				pixel->g = g;
				pixel->b = b;
			}else{
				double mina = 1 - a;
				pixel->r = pixel->r * mina + r * a;
				pixel->g = pixel->g * mina + g * a;
				pixel->b = pixel->b * mina + b * a;
			}
	}
}

void drawLine(xy_t p1, xy_t p2, int r, int g, int b, double a)
{
	int x1 = p1.x, y1 = p1.y;
	int x2 = p2.x, y2 = p2.y;
	if(x1 == x2 && y1 == y2){
		if(x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT){
			pixelRGB_t *pixel = &pixels[x1 + y1 * WIDTH];
			if(a == 1){
				pixel->r = r;
				pixel->g = g;
				pixel->b = b;
			}else{
				double mina = 1 - a;
				pixel->r = pixel->r * mina + r * a;
				pixel->g = pixel->g * mina + g * a;
				pixel->b = pixel->b * mina + b * a;
			}
		}
		return;
	}
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = x1 < x2 ? 1 : -1;
	int sy = y1 < y2 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;

	while(true){
		if(x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT){
			pixelRGB_t *pixel = &pixels[x1 + y1 * WIDTH];
			if(a == 1){
				pixel->r = r;
				pixel->g = g;
				pixel->b = b;
			}else{
				double mina = 1 - a;
				pixel->r = pixel->r * mina + r * a;
				pixel->g = pixel->g * mina + g * a;
				pixel->b = pixel->b * mina + b * a;
			}
		}

		if(x1 == x2 && y1 == y2){
			break;
		}
		int err2 = err;
		if(err2 > -dx) {
			err -= dy;
			x1 += sx;
		}
		if(err2 < dy) {
			err += dx;
			y1 += sy;
		}
	}
}

void drawCircle(xy_t p, int radius, int r, int g, int b, double a)
{
	int x = radius;
	int y = 0;
	int error = 1 - x;

	while(x >= y){
		drawPixel(x + p.x, y + p.y, r, g, b, a);
		drawPixel(y + p.x, x + p.y, r, g, b, a);
		drawPixel(-x + p.x, y + p.y, r, g, b, a);
		drawPixel(-y + p.x, x + p.y, r, g, b, a);
		drawPixel(-x + p.x, -y + p.y, r, g, b, a);
		drawPixel(-y + p.x, -x + p.y, r, g, b, a);
		drawPixel(x + p.x, -y + p.y, r, g, b, a);
		drawPixel(y + p.x, -x + p.y, r, g, b, a);
		y++;
		if(error < 0){
			error += 2 * y + 1;
		}else{
			x--;
			error += 2 * (y - x) + 1;
		}
	}
}

void drawRightTriangle(int left, int right, int top, int bottom, bool flippedtop, bool flippedleft, int r, int g, int b)
{
	if(top == bottom){
		return;
	}

	double angle = (right - left) / (double)(top - bottom);

	if(flippedleft){
		if(flippedtop){
			int y;
			for(y = bottom; y < top; y++){
				hline(y, left, right - (y - bottom) * angle, r, g, b, 1);
			}
		}else{
			int y;
			for(y = bottom; y < top; y++){
				hline(y, left, right + (y - top) * angle, r, g, b, 1);
			}
		}
	}else{
		if(flippedtop){
			int y;
			for(y = bottom; y < top; y++){
				hline(y, left + (y - bottom) * angle, right, r, g, b, 1);
			}
		}else{
			int y;
			for(y = bottom; y < top; y++){
				hline(y, left - (y - top) * angle, right, r, g, b, 1);
			}
		}
	}
}

xy_t vectorUnit(xy_t p)
{
	double len = sqrt(p.x * p.x + p.y * p.y);
	p.x /= len;
	p.y /= len;

	return p;
}

bool vectorIsEqual(xy_t p1, xy_t p2)
{
	return p1.x - FL_ERROR < p2.x && p1.x + FL_ERROR > p2.x && p1.y - FL_ERROR < p2.y && p1.y + FL_ERROR > p2.y;
}

double vectorDotProduct(xy_t p1, xy_t p2)
{
	return p1.x * p2.x + p1.y * p2.y;
}

double vectorCrossProduct(xy_t p1, xy_t p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

bool vectorIsBetween(xy_t p, xy_t left, xy_t right)
{
	double leftRight = vectorDotProduct(left, right);

	return leftRight < vectorDotProduct(left, p) && leftRight < vectorDotProduct(right, p);
}

xy_t vectorProject(xy_t p1, xy_t p2)
{
	xy_t normal = vectorUnit(p2);
	double scalar = vectorDotProduct(p1, normal);
	normal.x *= scalar;
	normal.y *= scalar;

	return normal;
}

double vectorProjectScalar(xy_t p1, xy_t p2)
{
	return vectorDotProduct(p1, vectorUnit(p2));
}

bool pointIsLeft(xy_t p, xy_t l1, xy_t l2)
{
	return (l2.x - l1.x) * (p.y - l1.y) > (l2.y - l1.y) * (p.x - l1.x);
}

int segmentSegmentIntersect(xy_t p1, xy_t p2, xy_t p3, xy_t p4, xy_t *p)
{
	double denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
	double n1 = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
	double n2 = (p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x);

	if(fabs(n1) < FL_ERROR && fabs(n2) < FL_ERROR && fabs(denom) < FL_ERROR){
		p->x = (p1.x + p2.x) / 2;
		p->y = (p1.y + p2.y) / 2;
		return 1;
	}

	if(fabs(denom) < FL_ERROR){
		return 0;
	}

	n1 /= denom;
	n2 /= denom;
	if(n1 < -FL_ERROR || n1 > 1.001f || n2 < -FL_ERROR || n2 > 1.001f){
		return 0;
	}

	p->x = p1.x + n1 * (p2.x - p1.x);
	p->y = p1.y + n1 * (p2.y - p1.y);
	return 1;
}

// http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282
int lineSegmentIntersect(xy_t p, xy_t r, xy_t q, xy_t q1, xy_t *result)
{
	xy_t s;
	s.x = q1.x - q.x;
	s.y = q1.y - q.y;

	xy_t diff;
	diff.x = q.x - p.x;
	diff.y = q.y - p.y;

	double denom = vectorCrossProduct(r, s);
	double u = vectorCrossProduct(diff, r);
	if(fabs(denom) < FL_ERROR){
		if(fabs(u) < FL_ERROR){
			result->x = (p.x + q.x) / 2;
			result->y = (p.y + q.y) / 2;
			return 1;
		}else{
			return 0;
		}
	}

	u /= denom;
	if(u < -FL_ERROR || u > 1.0 + FL_ERROR){
		return 0;
	}

	result->x = q.x + u * s.x;
	result->y = q.y + u * s.y;
	return 1;
}

int segmentCircleIntersect(xy_t p1, xy_t p2, xy_t circle, double radius, xy_t *p)
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
		return 1;
	}else{
		return 0;
	}
}

void printSectorInfo(unsigned int id)
{
	sector_t sect = sectors[id];

	printf("Sector \"%d\", floor: %g, ceil: %g\n", id, sect.floor.start.z, sect.ceil.start.z);
	if(sect.npoints > 0){
		printf("Vertices: ");
	}
	unsigned int i;
	for(i = 0; i < sect.npoints; i++){
		printf("(%g,%g) ", sect.vertex[i].x, sect.vertex[i].y);
	}

	if(sect.nneighbors > 0){
		printf("\nNeighbors: ");
	}
	for(i = 0; i < sect.nneighbors; i++){
		printf("%d ", sect.neighbors[i]);
	}
	printf("\n\n");
}

int findNeighborSector(unsigned int current, xy_t v1, xy_t v2)
{
	sector_t s1 = sectors[current];
	unsigned int i;
	for(i = 0; i < s1.nneighbors; i++){
		sector_t s2 = sectors[s1.neighbors[i]];
		unsigned int j, found = 0;
		for(j = 0; j < s2.npoints; j++){
			if(s2.vertex[j].x == v1.x && s2.vertex[j].y == v1.y){
				found++;
				if(found >= 2){
					return s1.neighbors[i];
				}
			}else if(s2.vertex[j].x == v2.x && s2.vertex[j].y == v2.y){
				found++;
				if(found >= 2){
					return s1.neighbors[i];
				}
			}
		}
	}

	return -1;
}

void clipPointToCamera(xy_t camleft, xy_t camright, xy_t *p1, xy_t p2)
{
	if(p1->y < 0){
		p1->x += -p1->y * (p2.x - p1->x) / (p2.y - p1->y);
		p1->y = 0;
	}

	xy_t cam;
	if(pointIsLeft(*p1, (xy_t){0, 0}, camleft)){
		cam = camleft;
	}else{
		cam = camright;
	}
	
	lineSegmentIntersect((xy_t){0, 0}, cam, *p1, p2, p1);
}

void populateLookupTables()
{
	unsigned int i;
	for(i = 1; i < HHEIGHT; i++){
		yLookup[i] = HEIGHT / (double)(i * 2.0f);
	}
}

void renderWall(xy_t left, xy_t right, double camlen, double top, double bottom, double above, double beneath)
{
	if(left.y <= 1 || right.y <= 1){
		return;
	}

	// Near plane is 1, find x position on the plane
	double projleftx = (left.x / left.y) * player.fov;
	double projrightx = (right.x / right.y) * player.fov;

	// Convert to screen coordinates
	int screenleftx = HWIDTH + projleftx * HWIDTH;
	int screenrightx = HWIDTH + projrightx * HWIDTH;
	if(screenleftx == screenrightx){
		return;
	}

	// Divide by the y value to get the distance and use that to calculate the height
	double eyeheight = player.pos.z - player.height;
	double projtoplefty = (top + eyeheight) / left.y;
	double projbotlefty = (bottom + eyeheight) / left.y;
	double projtoprighty = (top + eyeheight) / right.y;
	double projbotrighty = (bottom + eyeheight) / right.y;

	int screentoplefty = HHEIGHT - projtoplefty * HHEIGHT;
	int screenbotlefty = HHEIGHT - projbotlefty * HHEIGHT;
	int screentoprighty = HHEIGHT - projtoprighty * HHEIGHT;
	int screenbotrighty = HHEIGHT - projbotrighty * HHEIGHT;

#if 0

	// Render ceiling and top triangle wall
	int topy;
	if(screentoplefty > screentoprighty){
		drawRightTriangle(screenleftx, screenrightx, screentoplefty, screentoprighty, true, true, 64, 64, 64);

		drawRightTriangle(screenleftx, screenrightx, screentoplefty, screentoprighty, false, false, 64, 32, 64);

		topy = screentoprighty;
	}else{
		drawRightTriangle(screenleftx, screenrightx, screentoprighty, screentoplefty, true, false, 64, 64, 64);

		drawRightTriangle(screenleftx, screenrightx, screentoprighty, screentoplefty, false, true, 64, 32, 64);

		topy = screentoplefty;
	}
	if(topy >= 0 && topy < HEIGHT){
		unsigned int y;
		if(topy < above){
			above = topy;
		}
		for(y = 0; y < above; y++){
			hline(y, screenleftx, screenrightx, 64, 64, 64, 1);
		}
	}

	// Render floor and bottom triangle wall
	int boty;
	if(screenbotlefty < screenbotrighty){
		drawRightTriangle(screenleftx, screenrightx, screenbotrighty, screenbotlefty, false, true, 64, 64, 64);

		drawRightTriangle(screenleftx, screenrightx, screenbotrighty, screenbotlefty, true, false, 64, 32, 64);

		boty = screenbotrighty;
	}else{
		drawRightTriangle(screenleftx, screenrightx, screenbotlefty, screenbotrighty, false, false, 64, 64, 64);

		drawRightTriangle(screenleftx, screenrightx, screenbotlefty, screenbotrighty, true, true, 64, 32, 64);

		boty = screenbotlefty;
	}
	if(boty >= 0 && boty < HEIGHT){
		unsigned int y;
		if(boty > beneath){
			beneath = boty;
		}
		for(y = beneath; y < HEIGHT; y++){
			hline(y, screenleftx, screenrightx, 64, 64, 64, 1);
		}
	}

	// boty should always be the bottom one
	boty = min(screenbotrighty, screenbotlefty);
	topy = max(screentoprighty, screentoplefty);
	if(boty < topy){
		return;
	}

	topy = min(max(topy, 0), HEIGHT);
	boty = min(max(boty, 0), HEIGHT);

	unsigned int y;
	for(y = topy; y < boty; y++){
		hline(y, screenleftx, screenrightx, 64, 32, 64, 1);
	}
#endif

	drawLine((xy_t){(double)screenleftx, (double)screentoplefty}, (xy_t){(double)screenrightx, (double)screentoprighty}, 255, 255, 255, 1);
	drawLine((xy_t){(double)screenleftx, (double)screenbotlefty}, (xy_t){(double)screenrightx, (double)screenbotrighty}, 255, 255, 255, 1);

	drawLine((xy_t){(double)screenleftx, (double)screentoplefty}, (xy_t){(double)screenleftx, (double)screenbotlefty}, 255, 255, 255, 1);
	drawLine((xy_t){(double)screenrightx, (double)screentoprighty}, (xy_t){(double)screenrightx, (double)screenbotrighty}, 255, 255, 255, 1);
}

void renderSector(unsigned int id, xy_t campos, xy_t camleft, xy_t camright, double camlen, unsigned int oldId, xy_t leftWall, xy_t rightWall)
{
	unsigned int i;
	for(i = 0; i < sectors[id].nvisited; i++){
		if(sectors[id].visited[i] == oldId || sectors[id].visited[i] == id){
			return;
		}
	}
	if(sectors[id].nvisited == 0){
		sectors[id].visited = (unsigned int*)malloc(sizeof(unsigned int));
		sectors[id].visited[0] = oldId;
		sectors[id].nvisited = 1;
	}else{
		sectors[id].visited = (unsigned int*)malloc(++sectors[id].nvisited * sizeof(unsigned int));
		sectors[id].visited[sectors[id].nvisited - 1] = oldId;
	}

	double sina = sin(player.angle);
	double cosa = cos(player.angle);
	xy_t camleftnorm = vectorUnit(camleft);
	xy_t camrightnorm = vectorUnit(camright);

	sector_t sect = sectors[id];
	for(i = 0; i < sect.npoints; i++){
		xy_t p1, p2;
		if(i > 0){
			p1 = sect.vertex[i];
			p2 = sect.vertex[i - 1];
		}else{
			p1 = sect.vertex[0];
			p2 = sect.vertex[sect.npoints - 1];
		}

		if((p1.x == leftWall.x && p1.y == leftWall.y && p2.x == rightWall.x && p2.y == rightWall.y) ||
				(p2.x == leftWall.x && p2.y == leftWall.y && p1.x == rightWall.x && p1.y == rightWall.y)){
			continue;
		}

		xy_t v1 = {campos.x - p1.x, campos.y - p1.y};
		xy_t v2 = {campos.x - p2.x, campos.y - p2.y};

		// 2D transformation matrix for rotations
		xy_t tv1 = {.y = sina * v1.x + cosa * v1.y};
		xy_t tv2 = {.y = sina * v2.x + cosa * v2.y};

		// Clip everything behind the player
		if(tv1.y <= 0 && tv2.y <= 0){
			continue;
		}

		tv1.x = cosa * v1.x - sina * v1.y;
		tv2.x = cosa * v2.x - sina * v2.y;

		// Clip everything outside of the field of view
		xy_t uv1 = vectorUnit(tv1);
		xy_t uv2 = vectorUnit(tv2);
		bool notbetween1 = !vectorIsBetween(uv1, camleftnorm, camrightnorm);
		bool notbetween2 = !vectorIsBetween(uv2, camleftnorm, camrightnorm);
		if(notbetween1 && notbetween2){
			// Remove them if they both lie on the same side
			if(pointIsLeft(tv1, (xy_t){0, 0}, camleftnorm) && pointIsLeft(tv2, (xy_t){0, 0}, camleftnorm)){
				continue;
			}else if(!pointIsLeft(tv1, (xy_t){0, 0}, camrightnorm) && !pointIsLeft(tv2, (xy_t){0, 0}, camrightnorm)){
				continue;
			}else if(tv1.y - ((tv2.y - tv1.y) / (tv2.x - tv1.x)) * tv1.x <= 0){
				// Use the function y = ax + b to determine if the line is above or under the player and clip if it's under
				continue;
			}
		}

		v1.x = tv1.x;
		v1.y = tv1.y;
		if(notbetween1 && !vectorIsEqual(tv1, camleft) && !vectorIsEqual(tv1, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &v1, tv2);
		}
		if(notbetween2 && !vectorIsEqual(tv2, camleft) && !vectorIsEqual(tv2, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &tv2, tv1);
		}
		tv1.x = v1.x;
		tv1.y = v1.y;

		double cross = vectorCrossProduct(tv1, tv2);
		if(cross > 0){
			xy_t temp = tv1;
			tv1 = tv2;
			tv2 = temp;
		}

		int near;
		if((near = findNeighborSector(id, p1, p2)) != -1){
			renderSector(near, campos, tv1, tv2, camlen, id, p1, p2);
			
			// Check if we need to render the bottom or upper edge
			sector_t neighbor = sectors[near];
			if(neighbor.ceil.start.z < sect.ceil.start.z){
				renderWall(tv1, tv2, camlen, sect.ceil.start.z, neighbor.ceil.start.z, HEIGHT, neighbor.floor.start.z);
			}
			if(neighbor.floor.start.z > sect.floor.start.z){
				renderWall(tv1, tv2, camlen, neighbor.ceil.start.z, sect.ceil.start.z, HEIGHT, neighbor.floor.start.z);
			}
		}else{
			renderWall(tv1, tv2, camlen, sect.ceil.start.z, sect.floor.start.z, HEIGHT, 0);
		}
	}
}

void renderMap()
{
	xy_t pos;
	pos.x = player.pos.x / 2;
	pos.y = player.pos.y / 2;

	xy_t lookat;
	lookat.x = pos.x - sin(player.angle) * 10;
	lookat.y = pos.y - cos(player.angle) * 10;

	drawLine(pos, lookat, 255, 0, 0, 1);

	unsigned int i;
	for(i = 0; i < nsectors; i++){
		sector_t sect = sectors[i];
		unsigned int j;
		for(j = 0; j < sect.npoints; j++){
			xy_t v1, v2;
			if(j > 0){
				v1 = sect.vertex[j];
				v2 = sect.vertex[j - 1];
			}else{
				v1 = sect.vertex[0];
				v2 = sect.vertex[sect.npoints - 1];
			}

			v1.x /= 2;
			v1.y /= 2;
			v2.x /= 2;
			v2.y /= 2;
			drawLine(v1, v2, 0, 255, 0, 0.5f);
		}
	}
}

void renderScene()
{
	unsigned int i;
	for(i = 0; i < nsectors; i++){
		if(sectors[i].nvisited > 0){
			free(sectors[i].visited);
			sectors[i].nvisited = 0;
		}
	}

	xy_t camleft = {-200, 200};
	xy_t camright = {200, 200};

	xy_t camunit = vectorUnit(camright);
	player.fov = (camunit.x * camunit.y) * 2;

	renderSector(player.sector, (xy_t){player.pos.x, player.pos.y}, camleft, camright, 1000, player.sector, (xy_t){-1, -1}, (xy_t){-1, -1});

#ifdef RENDER_MAP
	renderMap();
#endif
}

void render()
{
	renderScene();

	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f, 1.0f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int i;
	for(i = 0; i < WIDTH * HEIGHT; i++){
		pixels[i].r = pixels[i].g = pixels[i].b = 0;
	}
}

void movePlayer(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed, bool spacePressed)
{
	if(upPressed){
		player.vel.x += cos(player.angle + M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle + M_PI / 2) * PLAYER_SPEED;
	}
	if(downPressed){
		player.vel.x += cos(player.angle - M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle - M_PI / 2) * PLAYER_SPEED;
	}
	if(leftPressed){
#ifdef USE_MOUSE
			player.vel.x += cos(player.angle) * PLAYER_SPEED;
			player.vel.y -= sin(player.angle) * PLAYER_SPEED;
#else
			player.angle -= 0.035f;
#endif
	}
	if(rightPressed){
#ifdef USE_MOUSE
			player.vel.x += cos(player.angle - M_PI) * PLAYER_SPEED / 2;
			player.vel.y -= sin(player.angle - M_PI) * PLAYER_SPEED / 2;
#else
			player.angle += 0.035f;
#endif
	}
	if(spacePressed){
		if(player.pos.z >= 0){
			player.vel.z = PLAYER_JUMP;
			player.pos.z = -0.1f;
		}
	}

	unsigned int found = 0;
	if(player.vel.x > FL_ERROR || player.vel.x < -FL_ERROR || player.vel.y > FL_ERROR || player.vel.y < -FL_ERROR){
		sector_t sect = sectors[player.sector];
		unsigned int i;
		for(i = 0; i < sect.npoints; i++){
			xy_t v1, v2;
			if(i > 0){
				if(sect.npoints == 2){
					break;
				}
				v1 = sect.vertex[i];
				v2 = sect.vertex[i - 1];
			}else{
				v1 = sect.vertex[0];
				v2 = sect.vertex[sect.npoints - 1];
			}
			// Find which segment the player wants to pass throught
			xy_t isect;
			if(!segmentSegmentIntersect(v1, v2, (xy_t){player.pos.x, player.pos.y}, (xy_t){player.pos.x + player.vel.x, player.pos.y + player.vel.y}, &isect)){
				continue;
			}

			unsigned int j;
			for(j = 0; j < sect.nneighbors; j++){
				sector_t neighbor = sectors[sect.neighbors[j]];
				found = 0;
				unsigned int k;
				for(k = 0; k < neighbor.npoints; k++){
					if(v1.x == neighbor.vertex[k].x && v1.y == neighbor.vertex[k].y){
						found++;
					}else	if(v2.x == neighbor.vertex[k].x && v2.y == neighbor.vertex[k].y){
						found++;
					}else{
						continue;
					}
					if(found == 2){
						player.sector = sect.neighbors[j];
						goto foundAll;
					}
				}
			}
foundAll:
			if(found < 2){				
				xy_t proj = vectorProject((xy_t){player.pos.x + player.vel.x - v2.x, player.pos.y + player.vel.y - v2.y}, (xy_t){v1.x - v2.x, v1.y - v2.y});

				player.pos.x = proj.x + v2.x - player.vel.x;
				player.pos.y = proj.y + v2.y - player.vel.y;
				player.vel.x = 0;
				player.vel.y = 0;
			}
		}

		player.pos.x += player.vel.x;
		player.pos.y += player.vel.y;
		player.vel.x *= PLAYER_FRICTION;
		player.vel.y *= PLAYER_FRICTION;
	}
	if(player.pos.z < -FL_ERROR){
		player.pos.z += player.vel.z;
		player.vel.z += PLAYER_GRAVITY;
	}else{
		player.pos.z = 0;
	}
#ifdef USE_MOUSE
	player.angle += (ccWindowGetMouse().x - HWIDTH) / 1000.0f;
	player.yaw += (ccWindowGetMouse().y - HHEIGHT) / 1000.0f;
	ccWindowMouseSetPosition((ccPoint){HWIDTH, HHEIGHT});
#endif
}

void load(char *map)
{
	FILE *fp;
	char *line, *ptr;
	int index, index2, scanlen, nverts;
	size_t len;
	ssize_t read;
	xy_t vert, *verts;
	sector_t *sect;

	line = NULL;
	verts = NULL;
	len = 0;
	nverts = 0;

	fp = fopen(map, "rt");
	if(!fp) {
		printf("Couldn't open: %s\n", map);
		exit(1);
	}

	/* TODO: replace GNU readline with a cross platform solution */
	while((read = getline(&line, &len, fp)) != -1) {
		switch(line[0]){
			case 'v':
				ptr = line;
				sscanf(ptr, "%*s %lf%n", &vert.y, &scanlen);
				while(sscanf(ptr += scanlen, "%lf%n", &vert.x, &scanlen) == 1){
					verts = (xy_t*)realloc(verts, ++nverts * sizeof(*verts));
					verts[nverts - 1] = vert;
				}
				break;
			case 's':
				sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(*sectors));
				sect = sectors + nsectors - 1;

				ptr = line;
				sect->npoints = 0;
				sect->vertex = NULL;
				sect->visited = NULL;
				sect->nvisited = 0;
				sect->nneighbors = 0;
				sect->neighbors = NULL;

				sscanf(ptr, "%*s %lf %lf %lf %u %lf %lf %lf %u%n", &sect->floor.start.z, &sect->floor.slope, &sect->floor.angle, &index,
						&sect->ceil.start.z, &sect->ceil.slope, &sect->ceil.angle, &index2, &scanlen);
				sect->floor.start.x = verts[index].x;
				sect->floor.start.y = verts[index].y;
				sect->ceil.start.x = verts[index2].x;
				sect->ceil.start.y = verts[index2].y;
				while(sscanf(ptr += scanlen, "%d%n", &index, &scanlen) == 1){
					sect->vertex = (xy_t*)realloc(sect->vertex, ++sect->npoints * sizeof(*sect->vertex));
					sect->vertex[sect->npoints - 1] = verts[index];
				}

				sscanf(ptr += scanlen, "%*c%n", &scanlen);
				while(sscanf(ptr += scanlen, "%u%n", &index, &scanlen) == 1){
					sect->neighbors = (unsigned int*)realloc(sect->neighbors, ++sect->nneighbors * sizeof(*sect->neighbors));
					sect->neighbors[sect->nneighbors - 1] = index;
				}

				printSectorInfo(nsectors - 1);
				break;
			case 'p':
				sscanf(line, "%*s %lf %lf %lf %lf", &player.height, &player.pos.x, &player.pos.y, &player.pos.z);
				player.angle = M_PI / 2;
				player.sector = 0;
				player.yaw = 0;
				player.radius = 5;
				break;
		}
	}

	fclose(fp);
	free(line);
	free(verts);
}

int main(int argc, char **argv)
{
	bool loop, upPressed, downPressed, leftPressed, rightPressed, spacePressed;

	load(argv[1]);

	populateLookupTables();

	ccDisplayInitialize();

	ccWindowCreate((ccRect){0, 0, WIDTH, HEIGHT}, "3D", CC_WINDOW_FLAG_NORESIZE);
	ccWindowMouseSetCursor(CC_CURSOR_NONE);

	ccGLContextBind();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	loop = true;
	spacePressed = leftPressed = rightPressed = upPressed = downPressed = false;
	while(loop){
		while(ccWindowEventPoll()){
			if(ccWindowEventGet().type == CC_EVENT_WINDOW_QUIT){
				loop = false;
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_DOWN){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_ESCAPE:
						loop = false;
						break;
					case CC_KEY_W:
					case CC_KEY_UP:
						upPressed = true;
						break;
					case CC_KEY_S:
					case CC_KEY_DOWN:
						downPressed = true;
						break;
					case CC_KEY_A:
					case CC_KEY_LEFT:
						leftPressed = true;
						break;
					case CC_KEY_D:
					case CC_KEY_RIGHT:
						rightPressed = true;
						break;
					case CC_KEY_SPACE:
						spacePressed = true;
						break;
				}
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_UP){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_W:
					case CC_KEY_UP:
						upPressed = false;
						break;
					case CC_KEY_S:
					case CC_KEY_DOWN:
						downPressed = false;
						break;
					case CC_KEY_A:
					case CC_KEY_LEFT:
						leftPressed = false;
						break;
					case CC_KEY_D:
					case CC_KEY_RIGHT:
						rightPressed = false;
						break;
					case CC_KEY_SPACE:
						spacePressed = false;
						break;
				}
			}
		}

		movePlayer(upPressed, downPressed, leftPressed, rightPressed, spacePressed);

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
