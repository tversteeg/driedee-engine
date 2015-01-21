#include <stdio.h>
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
	xyz *pos, vel;
	float angle;
	unsigned int sector;
} player;

GLuint texture;
pixelRGB pixels[WIDTH * HEIGHT];
sector *sectors = NULL;
unsigned int nsectors = 0;

void render()
{
	unsigned int i;

	for(i = 0; i < WIDTH * HEIGHT; i++){
		pixels[i].r = i % 256;
		pixels[i].g = i % 256;
		pixels[i].b = i % 256;
	}

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
}

void load(char *map)
{
	FILE *fp;

	fp = fopen(map, "rt");
	if(!fp) {
		printf("Couldn't open: %s\n", map);
		exit(1);
	}

	fclose(fp);
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
