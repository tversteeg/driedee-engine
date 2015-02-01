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

#include <ccNoise/ccNoise.h>
#include <ccRandom/ccRandom.h>

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

#define CROSS_PRODUCT(p1, p2) (p1.x * p2.y - p1.y * p2.x)

typedef struct {
	unsigned char r, g, b;
} pixelRGB;

typedef struct {
	float x, y;
} xy;

typedef struct {
	float x, y, z;
} xyz;

typedef struct {
	xy *vertex;
	float floor, ceil;
	unsigned int npoints, *neighbors;
} sector;

struct player {
	xyz pos, vel;
	float angle;
	unsigned int sector;
} player;

GLuint texture;
pixelRGB pixels[WIDTH * HEIGHT];
sector *sectors = NULL;
unsigned int nsectors = 0;

int lineIntersect(xy p1, xy p2, xy p3, xy p4, xy *p)
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

void drawLine(xy p1, xy p2, int r, int g, int b, float a)
{
	pixelRGB *pixel;
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

void renderWalls()
{
	unsigned int i, j;
	sector sect;
	xy v1, v2, tv1, tv2;
	float cosa, sina;

	for(i = 0; i < nsectors; i++){
		sect = sectors[i];
		for(j = 0; j < sect.npoints; j++){
			if(j > 0){
				if(sect.npoints == 2){
					break;
				}
				v1 = sect.vertex[j];
				v2 = sect.vertex[j - 1];
			}else{
				v1 = sect.vertex[0];
				v2 = sect.vertex[sect.npoints - 1];
			}

			v1.x = player.pos.x - v1.x;
			v1.y = player.pos.y - v1.y;
			v2.x = player.pos.x - v2.x;
			v2.y = player.pos.y - v2.y;

			sina = sin(player.angle);
			cosa = cos(player.angle);

			// 2D transformation matrix for rotations
			tv1.x = cosa * v1.x - sina * v1.y;
			tv1.y = sina * v1.x + cosa * v1.y;

			tv2.x = cosa * v2.x - sina * v2.y;
			tv2.y = sina * v2.x + cosa * v2.y;

			v1.x = HWIDTH - tv1.x;
			v1.y = HHEIGHT - tv1.y;
			v2.x = HWIDTH - tv2.x;
			v2.y = HHEIGHT - tv2.y;

			if(i == player.sector){
				drawLine(v1, v2, 255, 0, 0, 0.5f);
			}else{
				drawLine(v1, v2, 0, 255, 0, 0.5f);
			}
		}
	}

}

void render()
{
	unsigned int i;

	for(i = 0; i < WIDTH * HEIGHT; i++){
		pixels[i].r = pixels[i].g = pixels[i].b = 0;
	}

	renderWalls();

	// Render player on map
	drawLine((xy){HWIDTH, HHEIGHT}, (xy){HWIDTH, HHEIGHT - 20}, 255, 0, 255, 0.5f);
	drawLine((xy){HWIDTH, HHEIGHT}, (xy){HWIDTH - 100, HHEIGHT - 100}, 0, 0, 255, 0.25f);
	drawLine((xy){HWIDTH, HHEIGHT}, (xy){HWIDTH + 100, HHEIGHT - 100}, 0, 0, 255, 0.25f);

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
}

void movePlayer(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed)
{
	sector sect;
	xy v1, v2, isect;
	unsigned int i;

	if(upPressed){
		player.vel.x += cos(player.angle + M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle + M_PI / 2) * PLAYER_SPEED;
	}
	if(downPressed){
		player.vel.x += cos(player.angle - M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle - M_PI / 2) * PLAYER_SPEED;
	}
	if(leftPressed){
		player.vel.x += cos(player.angle - M_PI) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle - M_PI) * PLAYER_SPEED;
	}
	if(rightPressed){
		player.vel.x += cos(player.angle) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle) * PLAYER_SPEED;
	}

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
		if(!lineIntersect(v1, v2, (xy){player.pos.x, player.pos.y}, (xy){player.pos.x + player.vel.x, player.pos.y + player.vel.y}, &isect)){
			continue;
		}
		player.vel.x = 0;
		player.vel.y = 0;
	}

	player.pos.x += player.vel.x;
	player.pos.y += player.vel.y;
	player.vel.x *= PLAYER_FRICTION;
	player.vel.y *= PLAYER_FRICTION;
	player.angle += (ccWindowGetMouse().x - HWIDTH) / 1000.0f;
	ccWindowMouseSetPosition((ccPoint){HWIDTH, HHEIGHT});
}

void load(char *map)
{
	FILE *fp;
	char *line, *ptr;
	int index, scanlen, nverts;
	size_t len;
	ssize_t read;
	xy vert, *verts;
	sector *sect;

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
					verts = (xy*)realloc(verts, ++nverts * sizeof(*verts));
					verts[nverts - 1] = vert;
				}
				break;
			case 's':
				sectors = (sector*)realloc(sectors, ++nsectors * sizeof(*sectors));
				sect = sectors + nsectors - 1;

				ptr = line;
				sect->npoints = 0;
				sect->vertex = NULL;
				sscanf(ptr, "%*s %f %f%n", &sect->floor, &sect->ceil, &scanlen);
				while(sscanf(ptr += scanlen, "%d%n", &index, &scanlen) == 1){
					sect->vertex = (xy*)realloc(sect->vertex, ++sect->npoints * sizeof(*sect->vertex));
					sect->vertex[sect->npoints - 1] = verts[index];
				}
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

		movePlayer(upPressed, downPressed, leftPressed, rightPressed);

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
