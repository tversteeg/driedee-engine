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

#define WIDTH 800
#define HEIGHT 600
#define HWIDTH (WIDTH / 2)
#define HHEIGHT (HEIGHT / 2)

#define PLAYER_SPEED 1.0f
#define PLAYER_FRICTION 0.8f

typedef struct {
	unsigned char r, g, b;
} pixelRGB_t;

typedef struct {
	float x, y;
} xy_t;

typedef struct {
	float x, y, z;
} xyz_t;

typedef struct {
	xyz_t start;
	float angle, slope;
} plane_t;

typedef struct {
	xy_t *vertex;
	plane_t floor, ceil;
	unsigned int npoints, nneighbors, *neighbors, nvisited, *visited;
} sector_t;

struct player {
	xyz_t pos, vel;
	float angle;
	unsigned int sector;
} player;

GLuint texture;
pixelRGB_t pixels[WIDTH * HEIGHT];
sector_t *sectors = NULL;
unsigned int nsectors = 0;

void drawLine(xy_t p1, xy_t p2, int r, int g, int b, float a)
{
	pixelRGB_t *pixel;
	int x1, y1, x2, y2, dx, dy, sx, sy, err, err2;
	float mina;

	x1 = p1.x, y1 = p1.y;
	x2 = p2.x, y2 = p2.y;
	if(x1 == x2 && y1 == y2){
		if(x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT){
			pixel = &pixels[x1 + y1 * WIDTH];
			if(a == 1){
				pixel->r = r;
				pixel->g = g;
				pixel->b = b;
			}else{
				mina = 1 - a;
				pixel->r = pixel->r * mina + r * a;
				pixel->g = pixel->g * mina + g * a;
				pixel->b = pixel->b * mina + b * a;
			}
		}
		return;
	}
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);
	sx = x1 < x2 ? 1 : -1;
	sy = y1 < y2 ? 1 : -1;
	err = (dx > dy ? dx : -dy) / 2;

	while(true){
		if(x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT){
			pixel = &pixels[x1 + y1 * WIDTH];
			if(a == 1){
				pixel->r = r;
				pixel->g = g;
				pixel->b = b;
			}else{
				mina = 1 - a;
				pixel->r = pixel->r * mina + r * a;
				pixel->g = pixel->g * mina + g * a;
				pixel->b = pixel->b * mina + b * a;
			}
		}

		if(x1 == x2 && y1 == y2){
			break;
		}
		err2 = err;
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

xy_t vectorUnit(xy_t p)
{
	float len;

	len = sqrt(p.x * p.x + p.y * p.y);
	p.x /= len;
	p.y /= len;

	return p;
}

float vectorDotProduct(xy_t p1, xy_t p2)
{
	return p1.x * p2.x + p1.y * p2.y;
}

float vectorCrossProduct(xy_t p1, xy_t p2)
{
	return p1.x * p2.y - p1.y * p2.x;
}

bool pointIsLeft(xy_t p, xy_t l1, xy_t l2)
{
	return (l2.x - l1.x) * (p.y - l1.y) > (l2.y - l1.y) * (p.x - l1.x);
}

int lineLineIntersect(xy_t p1, xy_t p2, xy_t p3, xy_t p4, xy_t *p)
{
	float denom, n1, n2;

	denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
	n1 = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
	n2 = (p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x);

	if(fabs(n1) < 0.0001f && fabs(n2) < 0.0001f && fabs(denom) < 0.0001f){
		p->x = (p1.x + p2.x) / 2;
		p->y = (p1.y + p2.y) / 2;
		return 1;
	}

	if(fabs(denom) < 0.0001f){
		return 0;
	}

	n1 /= denom;
	n2 /= denom;
	if(n1 < 0 || n1 > 1 || n2 < 0 || n2 > 1){
		return 0;
	}

	p->x = p1.x + n1 * (p2.x - p1.x);
	p->y = p1.y + n1 * (p2.y - p1.y);
	return 1;
}

// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines 
// // intersect the intersection point may be stored in the floats i_x and i_y.
char get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
		float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y)
{
	float s1_x, s1_y, s2_x, s2_y;
	s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
	s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;

	float s, t;
	s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
	t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		// Collision detected
		if (i_x != NULL)
			*i_x = p0_x + (t * s1_x);
		if (i_y != NULL)
			*i_y = p0_y + (t * s1_y);
		return 1;
	}

	return 0; // No collision
}

char segmentDirIntersection(xy_t dir, xy_t p1, xy_t p2, xy_t *result)
{
	xy_t det, diff;
	float cross, divisor;

	diff.x = p1.x - p2.x;
	diff.y = p1.y - p2.y;

	cross = vectorCrossProduct(p1, p2);
	divisor = -dir.x * diff.y + dir.y * diff.x;
	if(divisor < 0.0001f && divisor > -0.00001f){
		return -1;
	}

	det.x = (diff.x + dir.x * cross) / divisor;
	det.y = (diff.y + dir.y * cross) / divisor;

	result->x = det.x;
	result->y = det.y;

	return 0;
}

// http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282
int lineSegmentIntersect(xy_t p, xy_t r, xy_t q, xy_t q1, xy_t *result)
{
	xy_t s, diff;
	float u, v, denom;

	s.x = q1.x - q.x;
	s.y = q1.y - q.y;

	diff.x = q.x - p.x;
	diff.y = q.y - p.y;

	denom = vectorCrossProduct(r, s);
	u = vectorCrossProduct(diff, r);
	v = vectorCrossProduct(diff, s);
	if(denom == 0){
		if(u == 0 && v == 0){
			result->x = (p.x + q.x) / 2;
			result->y = (p.y + q.y) / 2;
			return 1;
		}else{
			return 0;
		}
	}

	u /= denom;
	if(u < 0 || u > 1){
		return 0;
	}

	v /= denom;
	if(v < 0){
		return 0;
	}

	result->x = q.x + u * s.x;
	result->y = q.y + u * s.y;
	return 1;
}

bool vectorIsBetween(xy_t p, xy_t left, xy_t right)
{
	float leftRight;

	leftRight = vectorDotProduct(left, right);

	return leftRight < vectorDotProduct(left, p) && leftRight < vectorDotProduct(right, p);
}

xy_t vectorProject(xy_t p1, xy_t p2)
{
	float scalar;
	xy_t normal;

	normal = vectorUnit(p2);
	scalar = p1.x * normal.x + p1.y * normal.y;
	normal.x *= scalar;
	normal.y *= scalar;

	return normal;
}

void printSectorInfo(unsigned int id)
{
	sector_t sect;
	unsigned int i;

	sect = sectors[id];

	printf("Sector \"%d\", floor: %g, ceil: %g\n", id, sect.floor.start.z, sect.ceil.start.z);
	if(sect.npoints > 0){
		printf("Vertices: ");
	}
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
	int i, j;
	int found;
	sector_t s1, s2;

	s1 = sectors[current];
	for(i = 0; i < s1.nneighbors; i++){
		s2 = sectors[s1.neighbors[i]];
		found = 0;
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

void renderSector(unsigned int id, xy_t campos, xy_t camleft, xy_t camright, float camlen, unsigned int oldId, xy_t leftWall, xy_t rightWall)
{
	unsigned int i;
	int near;
	sector_t sect;
	xy_t v1, v2, tv1, tv2, uv1, uv2, camleftnorm, camrightnorm;
	float cosa, sina;
	bool notbetween1, notbetween2;

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

	sina = sin(player.angle);
	cosa = cos(player.angle);
	camleftnorm = vectorUnit(camleft);
	camrightnorm = vectorUnit(camright);

	drawLine((xy_t){HWIDTH - camleft.x, HHEIGHT - camleft.y}, (xy_t){HWIDTH - camleft.x - camleftnorm.x * 20, HHEIGHT - camleft.y - camleftnorm.y * 20}, 255, 0, 0, 0.9f);
	drawLine((xy_t){HWIDTH - camright.x, HHEIGHT - camright.y}, (xy_t){HWIDTH - camright.x - camrightnorm.x * 20, HHEIGHT - camright.y - camrightnorm.y * 20}, 0, 255, 0, 0.9f);

	sect = sectors[id];
	for(i = 0; i < sect.npoints; i++){
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

		if((v1.x == leftWall.x && v1.y == leftWall.y && v2.x == rightWall.x && v2.y == rightWall.y) ||
				(v2.x == leftWall.x && v2.y == leftWall.y && v1.x == rightWall.x && v1.y == rightWall.y)){
			continue;
		}

		v1.x = campos.x - v1.x;
		v1.y = campos.y - v1.y;
		v2.x = campos.x - v2.x;
		v2.y = campos.y - v2.y;

		// 2D transformation matrix for rotations
		tv1.x = cosa * v1.x - sina * v1.y;
		tv1.y = sina * v1.x + cosa * v1.y;

		tv2.x = cosa * v2.x - sina * v2.y;
		tv2.y = sina * v2.x + cosa * v2.y;

		// Clip everything behind the player
		if(tv1.y < 0 && tv2.y < 0){
			continue;
		}

		// Clip everything outside of the field of view
		uv1 = vectorUnit(tv1);
		uv2 = vectorUnit(tv2);
		notbetween1 = !vectorIsBetween(uv1, camleftnorm, camrightnorm);
		notbetween2 = !vectorIsBetween(uv2, camleftnorm, camrightnorm);
		if(notbetween1 && notbetween2){
			// Remove them if they both lie on the same side
			if(pointIsLeft(tv1, (xy_t){0, 0}, camleftnorm) && pointIsLeft(tv2, (xy_t){0, 0}, camleftnorm)){
				continue;
			}else if(!pointIsLeft(tv1, (xy_t){0, 0}, camrightnorm) && !pointIsLeft(tv2, (xy_t){0, 0}, camrightnorm)){
				continue;
			}else if(tv1.y - ((tv2.y - tv1.y) / (tv2.x - tv1.x)) * tv1.x < 0){
				// Use the function y = ax + b to determine if the line is above or under the player and clip if it's under
				continue;
			}
		}

		v1.x = tv1.x;
		v1.y = tv1.y;
		if(notbetween1){
			if(pointIsLeft(tv1, (xy_t){0, 0}, camleftnorm)){
				//if(get_line_intersection(0, 0, camleftnorm.x * 10000, camleftnorm.y * 10000, tv2.x, tv2.y, tv1.x, tv1.y, &v1.x, &v1.y) == 0){
				if(lineSegmentIntersect((xy_t){0, 0}, camleftnorm, tv2, tv1, &v1) == 0){
					drawLine((xy_t){HWIDTH - tv2.x - 5, HHEIGHT - tv2.y}, (xy_t){HWIDTH - v1.x - 5, HHEIGHT - v1.y}, 128, 255, 128, 1);
				}
			//}else if(get_line_intersection(0, 0, camrightnorm.x * 10000, camrightnorm.y * 10000, tv2.x, tv2.y, tv1.x, tv1.y, &v1.x, &v1.y) == 0){
			}else if(lineSegmentIntersect((xy_t){0, 0}, camrightnorm, tv2, tv1, &v1) == 0){
				drawLine((xy_t){HWIDTH - tv2.x + 5, HHEIGHT - tv2.y}, (xy_t){HWIDTH - v1.x + 5, HHEIGHT - v1.y}, 0, 255, 128, 1);
			}
		}
		if(notbetween2){
			if(pointIsLeft(tv2, (xy_t){0, 0}, camleftnorm)){
				if(lineSegmentIntersect((xy_t){0, 0}, camleftnorm, tv2, tv1, &tv2) == 0){
					drawLine((xy_t){HWIDTH - tv2.x - 5, HHEIGHT - tv2.y}, (xy_t){HWIDTH - v1.x - 5, HHEIGHT - v1.y}, 255, 128, 128, 1);
				}
			}else if(lineSegmentIntersect((xy_t){0, 0}, camrightnorm, tv2, tv1, &tv2) == 0){
				drawLine((xy_t){HWIDTH - tv2.x + 5, HHEIGHT - tv2.y}, (xy_t){HWIDTH - v1.x + 5, HHEIGHT - v1.y}, 255, 0, 128, 1);
			}
		}
		tv1.x = v1.x;
		tv1.y = v1.y;

		if(i > 0){
			v1 = sect.vertex[i];
			v2 = sect.vertex[i - 1];
		}else{
			v1 = sect.vertex[0];
			v2 = sect.vertex[sect.npoints - 1];
		}

		if((near = findNeighborSector(id, v1, v2)) != -1){
			if(vectorCrossProduct(tv1, tv2) < 0){
				renderSector(near, campos, tv1, tv2, camlen, id, v1, v2);
			}else{
				renderSector(near, campos, tv2, tv1, camlen, id, v2, v1);
			}
			//drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH - tv1.x, HHEIGHT - tv1.y}, 255, 255, 0, 0.1f);
			//drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH - tv2.x, HHEIGHT - tv2.y}, 255, 255, 0, 0.1f);
		}

		v1.x = HWIDTH - tv1.x;
		v1.y = HHEIGHT - tv1.y;
		v2.x = HWIDTH - tv2.x;
		v2.y = HHEIGHT - tv2.y;

		if(near != -1){
			drawLine(v1, v2, 0, 0, 255, 0.5f);
		}else if(id == player.sector){
			drawLine(v1, v2, 255, 255, 128, 0.5f);
		}else{
			drawLine(v1, v2, 128, 255, 255, 0.5f);
		}
	}
}

void renderWalls()
{
	unsigned int i;
	xy_t camleft, camright;

	for(i = 0; i < nsectors; i++){
		if(sectors[i].nvisited > 0){
			free(sectors[i].visited);
			sectors[i].nvisited = 0;
		}
	}

	camleft = (xy_t){-200, 200};
	camright = (xy_t){200, 200};
	renderSector(player.sector, (xy_t){player.pos.x, player.pos.y}, camleft, camright, 1000, player.sector, (xy_t){-1, -1}, (xy_t){-1, -1});
}

void render()
{
	unsigned int i;

	renderWalls();

	// Render player on map
	drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH, HHEIGHT - 20}, 255, 0, 255, 0.5f);
	drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH - 100, HHEIGHT - 100}, 0, 0, 255, 0.25f);
	drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH + 100, HHEIGHT - 100}, 0, 0, 255, 0.25f);

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

	for(i = 0; i < WIDTH * HEIGHT; i++){
		pixels[i].r = pixels[i].g = pixels[i].b = 0;
	}
}

void movePlayer(bool useMouse, bool upPressed, bool downPressed, bool leftPressed, bool rightPressed)
{
	sector_t sect, neighbor;
	xy_t v1, v2, isect, proj;
	unsigned int i, j, k, found;

	if(upPressed){
		player.vel.x += cos(player.angle + M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle + M_PI / 2) * PLAYER_SPEED;
	}
	if(downPressed){
		player.vel.x += cos(player.angle - M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle - M_PI / 2) * PLAYER_SPEED;
	}
	if(leftPressed){
		if(useMouse){
			player.vel.x += cos(player.angle - M_PI) * PLAYER_SPEED;
			player.vel.y -= sin(player.angle - M_PI) * PLAYER_SPEED;
		}else{
			player.angle += 0.05f;
		}
	}
	if(rightPressed){
		if(useMouse){
			player.vel.x += cos(player.angle) * PLAYER_SPEED;
			player.vel.y -= sin(player.angle) * PLAYER_SPEED;
		}else{
			player.angle -= 0.05f;
		}
	}

	if(player.vel.x > 0.01f || player.vel.x < -0.01f || player.vel.y > 0.01f || player.vel.y < -0.01f){
		sect = sectors[player.sector];
		for(i = 0; i < sect.npoints; i++){
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
			if(!lineLineIntersect(v1, v2, (xy_t){player.pos.x, player.pos.y}, (xy_t){player.pos.x + player.vel.x, player.pos.y + player.vel.y}, &isect)){
				continue;
			}
			for(j = 0; j < sect.nneighbors; j++){
				neighbor = sectors[sect.neighbors[j]];
				found = 0;
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
				proj = vectorProject((xy_t){player.pos.x + player.vel.x - v2.x, player.pos.y + player.vel.y - v2.y}, (xy_t){v1.x - v2.x, v1.y - v2.y});

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
	if(useMouse){
		player.angle += (ccWindowGetMouse().x - HWIDTH) / 1000.0f;
		ccWindowMouseSetPosition((ccPoint){HWIDTH, HHEIGHT});
	}
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
				sscanf(ptr, "%*s %f%n", &vert.y, &scanlen);
				while(sscanf(ptr += scanlen, "%f%n", &vert.x, &scanlen) == 1){
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

				sscanf(ptr, "%*s %f %f %f %u %f %f %f %u%n", &sect->floor.start.z, &sect->floor.slope, &sect->floor.angle, &index,
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
				sscanf(line, "%*s %f %f %f", &player.pos.x, &player.pos.y, &player.pos.z);
				player.angle = M_PI / 2;
				player.sector = 0;
				break;
		}
	}

	fclose(fp);
	free(line);
	free(verts);
}

int main(int argc, char **argv)
{
	bool loop, upPressed, downPressed, leftPressed, rightPressed;

	load(argv[1]);

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
	upPressed = downPressed = false;
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
				}
			}
		}

		movePlayer(false, upPressed, downPressed, leftPressed, rightPressed);

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
