#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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
#include "l_draw.h"
#include "l_sector.h"
#include "l_level.h"
#include "l_render.h"
#include "l_colors.h"
#include "l_png.h"

#define WIDTH 1280
#define HEIGHT 768

#define MAPWIDTH (WIDTH >> 3)
#define MAPHEIGHT (HEIGHT >> 3)
#define MAPSIZE ((MAPWIDTH) * (MAPHEIGHT))
char map[MAPSIZE];
char unitmap[MAPSIZE];

GLuint texture;
texture_t tex;
font_t font;

void generateMap()
{
	srand(time(NULL));

	int i;
	for(i = 0; i < MAPSIZE; i++){
		if(i % (MAPWIDTH) == 0 || i % (MAPWIDTH) == MAPWIDTH - 1 || i < MAPWIDTH || i > MAPSIZE - MAPWIDTH){
			map[i] = '#';
		}else if(rand() % 50 == 0){
			map[i] = '#';
		}else{
			map[i] = '.';
		}
	}

	int times;
	for(times = 0; times < 10; times++){
		for(i = MAPWIDTH; i < MAPSIZE - MAPWIDTH; i++){
			if((map[i - 1] == '#' || map[i + 1] == '#' || map[i - MAPWIDTH] == '#' || map[i + MAPWIDTH] == '#') && rand() % 10 == 0){
				map[i] = '#';
			}
		}
	}

#define ROOMS 40
	int rooms[ROOMS * 4];
	for(i = 0; i < ROOMS; i++){
		int width = rand() % 10 + 3;
		int height = rand() % 10 + 3;
		int x = rand() % (MAPWIDTH - width);
		int y = rand() % (MAPHEIGHT - height);

		rooms[i << 2] = x;
		rooms[(i << 2) + 1] = y;
		rooms[(i << 2) + 2] = width;
		rooms[(i << 2) + 3] = height;

		int j, k;
		for(j = x; j < x + width; j++){
			for(k = y; k < y + height; k++){
				if(k == y || j == x || k == y + height - 1 || j == x + width - 1){
					map[j + k * MAPWIDTH] = '*';
				}else{
					map[j + k * MAPWIDTH] = '%';
				}
			}
		}
	}

	for(i = 0; i < ROOMS; i++){
		int x = rooms[i << 2];
		int y = rooms[(i << 2) + 1];
		int width = rooms[(i << 2) + 2];
		int height = rooms[(i << 2) + 3];

		int j;
		for(j = rand() % 3 + 1; j >= 0; j--){
			int mx = x;
			int my = y;

			int side = rand() % 4;
			if(side == 0){
				mx += width - 1;
			}
			if(side == 2){
				my += height - 1;
			}

			if(side < 2){
				map[mx + (my + rand() % height) * MAPWIDTH] = '/';
			}else{
				map[mx + rand() % width + my * MAPWIDTH] = '/';
			}
		}
	}

#undef ROOMS

	for(i = 0; i < MAPSIZE; i++){
		if(map[i] == '.' && map[i + 1] == '.' && map[i - 1] == '.' && rand() % 100 == 0){
			unitmap[i] = 'R';
		}
	}

	unitmap[rand() % (MAPSIZE - MAPWIDTH * 2) + MAPWIDTH] = '@';
}

void renderMap()
{
	int i;
	for(i = 0; i < MAPSIZE; i++){
		int x = i % (MAPWIDTH);
		int y = i / (MAPWIDTH);
		if(unitmap[i] == '@'){
			drawLetter(&tex, &font, '@', x * 8, y * 8, COLOR_LIGHTBLUE);
		}else if(unitmap[i] != '\0'){
			drawLetter(&tex, &font, unitmap[i], x * 8, y * 8, COLOR_LIGHTRED);
		}else if(map[i] == '#'){
			drawLetter(&tex, &font, '#', x * 8, y * 8, COLOR_DARKGREEN);
		}else if(map[i] == '.'){
			drawLetter(&tex, &font, '.', x * 8, y * 8, COLOR_DARKBROWN);
		}else if(map[i] == '*'){
			drawLetter(&tex, &font, '*', x * 8, y * 8, COLOR_LIGHTGRAY);
		}else if(map[i] == '%'){
			drawLetter(&tex, &font, '.', x * 8, y * 8, COLOR_DARKGRAY);
		}else{
			drawLetter(&tex, &font, map[i], x * 8, y * 8, COLOR_RED);
		}
	}
}

void render()
{
	renderMap();

	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.pixels);

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

	clearTexture(&tex, COLOR_BLACK);
}

int main(int argc, char **argv)
{
	generateMap();

	initFont(&font, fontwidth, fontheight);
	loadFont(&font, '!', (bool*)fontdata);

	initTexture(&tex, WIDTH, HEIGHT);

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
			}
		}

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	return 0;
}
