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
	unsigned int npoints, nneighbors, *neighbors;
	bool renderred;
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

int lineIntersect(xy_t p1, xy_t p2, xy_t p3, xy_t p4, xy_t *p)
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

void printSectorInfo(unsigned int id)
{
	sector_t sect;
	unsigned int i;

	sect = sectors[id];

	printf("Sector \"%d\", floor: %g, ceil: %g, renderred: %s\n", id, sect.floor.start.z, sect.ceil.start.z, sect.renderred == true ? "true" : "false");
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

void renderSector(unsigned int id, xy_t campos, xy_t camleft, xy_t camright, float camlen)
{
	unsigned int i;
	int near;
	sector_t sect;
	xy_t v1, v2, tv1, tv2, uv1, uv2, camleftdist, camrightdist;
	float cosa, sina;

	if(sectors[id].renderred){
		return;
	}
	sectors[id].renderred = true;

	sina = sin(player.angle);
	cosa = cos(player.angle);
	camleftdist.x = camleft.x * camlen;
	camleftdist.y = camleft.y * camlen;
	camrightdist.x = camright.x * camlen;
	camrightdist.y = camright.y * camlen;

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
		if(tv1.y <= 0 && tv2.y <= 0){
			continue;
		}

		// Clip everything outside of the field of view
		uv1 = vectorUnit(tv1);
		uv2 = vectorUnit(tv2);
		if(!vectorIsBetween(uv1, camleft, camright) && !vectorIsBetween(uv2, camleft, camright)){
			if((uv1.x < 0 && uv2.x < 0) || (uv1.x > 0 && uv2.x > 0)){
				continue;
			}else if(tv1.y - ((tv2.y - tv1.y) / (tv2.x - tv1.x)) * tv1.x < 0){
				// Use the function y = ax + b to determine if the line is above or under the player and clip if it's under
				continue;
			}
		}

		// Find the vector to the frustrum
		if(tv1.x < -tv1.y){
			lineIntersect(tv1, tv2, (xy_t){0, 0}, camleftdist, &tv1);
		}
		if(tv1.x > tv1.y){
			lineIntersect(tv1, tv2, (xy_t){0, 0}, camrightdist, &tv1);
		}
		if(tv2.x < -tv2.y){
			lineIntersect(tv2, tv1, (xy_t){0, 0}, camleftdist, &tv2);
		}
		if(tv2.x > tv2.y){
			lineIntersect(tv2, tv1, (xy_t){0, 0}, camrightdist, &tv2);
		}

		if(i > 0){
			v1 = sect.vertex[i];
			v2 = sect.vertex[i - 1];
		}else{
			v1 = sect.vertex[0];
			v2 = sect.vertex[sect.npoints - 1];
		}
		
		if((near = findNeighborSector(id, v1, v2)) != -1){
			v1 = vectorUnit(tv1);
			v2 = vectorUnit(tv2);
			if(v1.x < v2.x){
				renderSector(near, campos, v1, v2, camlen);
			}else{
				renderSector(near, campos, v2, v1, camlen);
			}

			drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH - v1.x * 100, HHEIGHT - v1.y * 100}, 0, 0, 255, 0.25f);
			drawLine((xy_t){HWIDTH, HHEIGHT}, (xy_t){HWIDTH - v2.x * 100, HHEIGHT - v2.y * 100}, 0, 0, 255, 0.25f);
		}

		v1.x = HWIDTH - tv1.x;
		v1.y = HHEIGHT - tv1.y;
		v2.x = HWIDTH - tv2.x;
		v2.y = HHEIGHT - tv2.y;
		if(id == player.sector){
			drawLine(v1, v2, 255, 0, 0, 0.5f);
		}else{
			drawLine(v1, v2, 0, 255, 0, 0.5f);
		}

		if(near != -1){
			drawLine(v1, v2, 0, 0, 255, 0.5f);
		}
	}
}

void renderWalls()
{
	unsigned int i;

	for(i = 0; i < nsectors; i++){
		sectors[i].renderred = false;
	}
	renderSector(player.sector, (xy_t){player.pos.x, player.pos.y}, (xy_t){-1000, 1000}, (xy_t){1000, 1000}, 1000);
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
			if(!lineIntersect(v1, v2, (xy_t){player.pos.x, player.pos.y}, (xy_t){player.pos.x + player.vel.x, player.pos.y + player.vel.y}, &isect)){
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
				sect->renderred = false;
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
