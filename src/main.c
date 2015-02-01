#define _GNU_SOURCE
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

#define WIDTH 640
#define HEIGHT 480

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

void drawLine(xy p1, xy p2, int r, int g, int b)
{
	pixelRGB *pixel;
	if(p1.x == p2.x && p1.y == p2.y){
		if(p1.x >= 0 && p1.x < WIDTH && p1.y >= 0 && p1.y < HEIGHT){
			pixel = &pixels[(int)p1.x + (int)p1.y * WIDTH];
			pixel->r = r;
			pixel->g = g;
			pixel->b = b;
		}
	}
}

void render()
{
	unsigned int i;
	xy playerPos, lookDir;

	for(i = 0; i < WIDTH * HEIGHT; i++){
		pixels[i].r = pixels[i].g = pixels[i].b = 0;
	}

	playerPos.x = player.pos.x;
	playerPos.y = player.pos.y;
	lookDir.x = player.pos.x + (float)cos(player.angle) * 10;
	lookDir.y = player.pos.y + (float)sin(player.angle) * 10;
	drawLine(playerPos, lookDir, 255, 255, 255);

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
				break;
		}
	}

	fclose(fp);
	free(line);
	free(verts);
}

int main(int argc, char **argv)
{
	bool loop;

	load(argv[1]);

	ccDisplayInitialize();

	ccWindowCreate((ccRect){0, 0, WIDTH, HEIGHT}, "3D", CC_WINDOW_FLAG_NORESIZE);

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
			}
		}

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
