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

#include "PixelFont1.h"

#define WIDTH 800
#define HEIGHT 600

#define GRID_SIZE 10
#define MENU_HEIGHT 64

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

GLuint texture;
pixelRGB_t pixels[WIDTH * HEIGHT];
sector_t *sectors = NULL;
unsigned int nsectors = 0;

bool snaptogrid = false;

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

void drawLetter(char letter, int x, int y, int r, int g, int b, double a)
{
	char todraw = letter - '!';
	if(todraw < 0){
		return;
	}
	int drawpos = todraw * fontheight;

	int i;
	for(i = 0; i < 8; i++){
		int j;
		for(j = 0; j < 8; j++){
			if(fontdata[i + drawpos + j * fontwidth] != 0){
				drawPixel(x + i, y + j, r, g, b, a);
			}
		}
	}
}

void drawString(const char *string, int x, int y, int r, int g, int b, double a)
{
	int i;
	for(i = 0; string[i] != '\0'; i++){
		if(string[i] == '\n'){
			y += 8;
			x -= (i + 1) * fontheight;
		}else if(string[i] == '\t'){
			x += 16;
		}else{
			drawLetter(string[i], x + i * fontheight, y, r, g, b, a);
		}
	}
}

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
		return;
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
				break;
		}
	}
	fclose(fp);
	free(line);
	free(verts);
}

void drawGrid(int x, int y, int width, int height, int r, int g, int b, double a)
{
	int i;
	for(i = x; i < width; i += GRID_SIZE){
		vline(i, x, height, r, g, b, a);
	}
	for(i = y; i < height; i += GRID_SIZE){
		hline(i, y, width, r, g, b, a);
	}
}

void renderBackground()
{
	drawGrid(0, 0, WIDTH, HEIGHT - MENU_HEIGHT, 64, 64, 64, 1);
}

void renderMenu()
{
	hline(HEIGHT - MENU_HEIGHT, 0, WIDTH, 255, 255, 0, 1);

	char buffer[64];
	int pos = sprintf(buffer, "GRID SIZE: (%dx%d)", GRID_SIZE, GRID_SIZE);
	buffer[pos] = '\0';
	drawString(buffer, 8, HEIGHT - MENU_HEIGHT + 16, 255, 0, 0, 1);

	pos = sprintf(buffer, "(S) SNAP TO GRID: %s", snaptogrid ? "ON" : "OFF");
	buffer[pos] = '\0';
	drawString(buffer, 8, HEIGHT - MENU_HEIGHT + 24, 128, 0, 128, 1);
}

void renderMouse(int snap)
{
	int xmouse = ccWindowGetMouse().x;
	int ymouse = ccWindowGetMouse().y;

	if(snap > 1){
		xmouse = round(xmouse / snap) * snap;
		ymouse = round(ymouse / snap) * snap;
	}

	char buffer[64];
	int pos = sprintf(buffer, "MOUSE: (%d,%d)", xmouse, ymouse);
	buffer[pos] = '\0';
	drawString(buffer, 8, HEIGHT - MENU_HEIGHT + 8, 255, 0, 0, 1);

	vline(xmouse, ymouse - 5, ymouse + 5, 255, 255, 0, 1);
	hline(ymouse, xmouse - 5, xmouse + 5, 255, 255, 0, 1);
}

void renderMap()
{
	unsigned int i;
	for(i = 0; i < nsectors; i++){
		sector_t sect = sectors[i];
		unsigned int j;
		for(j = 0; j < sect.npoints; j++){
			xy_t v1 = sect.vertex[j];
			xy_t v2;
			if(j > 0){
				v2 = sect.vertex[j - 1];
			}else{
				v2 = sect.vertex[sect.npoints - 1];
			}
			if(v1.x >= 0 && v1.x < WIDTH && v1.y >= 0 && v1.y < HEIGHT - MENU_HEIGHT){
				drawCircle(v1, 2, 255, 255, 0, 1);
			}

			drawLine(v1, v2, 0, 128, 128, 1);
		}
	}
}

void render()
{	
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

int main(int argc, char **argv)
{
	load(argv[1]);

	ccDisplayInitialize();

	ccWindowCreate((ccRect){0, 0, WIDTH, HEIGHT}, "3D - editor", CC_WINDOW_FLAG_NORESIZE);
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

	bool loop = true;
	while(loop){
		while(ccWindowEventPoll()){
			if(ccWindowEventGet().type == CC_EVENT_WINDOW_QUIT){
				loop = false;
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_DOWN){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_ESCAPE:
						loop = false;
						break;
				}
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_UP){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_S:
						snaptogrid = !snaptogrid;
						break;
				}
			}
		}

		renderBackground();
		renderMap();
		renderMenu();
		if(!snaptogrid){
			renderMouse(1);
		}else{
			renderMouse(GRID_SIZE);
		}

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
