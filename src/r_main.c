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

#define VIEWPORTSIZE 40
#define MAPWIDTH 256
#define MAPHEIGHT 256
#define MAPSIZE ((MAPWIDTH) * (MAPHEIGHT))
char map[MAPSIZE];
char unitmap[MAPSIZE];

int getMapX(int pos)
{
	return pos % MAPWIDTH;
}
int getMapY(int pos)
{
	return pos / MAPWIDTH;
}
int getMapPos(int x, int y)
{
	return x + y * MAPWIDTH;
}

GLuint texture;
texture_t tex;
font_t font;

struct {
	int pos;
	int health, coins;
} player;

void generateMap()
{
	srand(time(NULL));

	int i;
	for(i = 0; i < MAPSIZE; i++){
		map[i] = unitmap[i] = '\0';
	}
	for(i = 0; i < MAPSIZE; i++){
		if(i % (MAPWIDTH) == 0 || i % (MAPWIDTH) == MAPWIDTH - 1 || i < MAPWIDTH || i > MAPSIZE - MAPWIDTH){
			map[i] = '#';
		}else if(rand() % 50 == 0){
			map[i] = '#';
		}else{
			map[i] = '.';
		}
	}

#define ROOMS (MAPSIZE / 512)
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
					map[getMapPos(j, k)] = '*';
				}else{
					map[getMapPos(j, k)] = '%';
				}
			}
		}
	}

	for(i = 0; i < ROOMS; i++){
		int x = rooms[i << 2];
		int y = rooms[(i << 2) + 1];
		int width = rooms[(i << 2) + 2];
		int height = rooms[(i << 2) + 3];

		int doors;
		if(width > 5 && height > 5){
			doors = rand() % 3 + 1;
		}else{
			doors = rand() % 2;
		}
		int j;
		for(j = doors; j >= 0; j--){
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
				map[getMapPos(mx, my + rand() % (height - 2) + 1)] = '%';
			}else{
				map[getMapPos(mx + rand() % (width - 2) + 1 , my)] = '%';
			}
		}
	}

#undef ROOMS

	int times;
	for(times = 0; times < 15; times++){
		for(i = MAPWIDTH; i < MAPSIZE - MAPWIDTH; i++){
			if(map[i] != '*' && (map[i - 1] == '#' || map[i + 1] == '#' || map[i - MAPWIDTH] == '#' || map[i + MAPWIDTH] == '#') && rand() % 10 == 0){
				map[i] = '#';
			}
		}
	}

	for(i = 0; i < MAPSIZE; i++){
		if(map[i] == '.' && map[i + 1] == '.' && map[i - 1] == '.' && rand() % 100 == 0){
			unitmap[i] = 'R';
		}
	}

	int pos;
	do{
		pos = rand() % MAPSIZE;
	} while(map[pos] != '.');
	player.pos = pos;
}

void renderGui()
{
	char *healthstr = (char*)malloc(30);
	snprintf(healthstr, 30, "Health: %d\nCoins:  %d", player.health, player.coins);
	drawString(&tex, &font, healthstr, VIEWPORTSIZE * 8 + 8, 8, COLOR_WHITE);

	free(healthstr);
}

char vismap[VIEWPORTSIZE][VIEWPORTSIZE];
bool checkvismap(int x, int y)
{
	if(vismap[x][y] != 0){
		return false;
	}
	if(x > 0){
		if(vismap[x - 1][y] != 0){
			return false;
		}
	}
	if(x < VIEWPORTSIZE - 1){
		if(vismap[x + 1][y] != 0){
			return false;
		}
	}
	if(y > 0){
		if(vismap[x][y - 1] != 0){
			return false;
		}
	}
	if(y < VIEWPORTSIZE - 1){
		if(vismap[x][y + 1] != 0){
			return false;
		}
	}

	return true;
}

void calculatelos()
{
	int x;
	for(x = 0; x < VIEWPORTSIZE / 2; x++){
		int y;
		for(y = 0; y < VIEWPORTSIZE / 2; y++){
			
		}
	}
}

void renderMap()
{
	int minx = getMapX(player.pos) - VIEWPORTSIZE / 2;
	minx = minx > 0 ? minx : 0;
	int maxx = minx + VIEWPORTSIZE;
	maxx = maxx < MAPWIDTH ? maxx : MAPWIDTH;
	int miny = getMapY(player.pos) - VIEWPORTSIZE / 2;
	miny = miny > 0 ? miny : 0;
	int maxy = miny + VIEWPORTSIZE;
	maxy = maxy < MAPHEIGHT ? maxy : MAPHEIGHT;

	memset(&vismap, 0, VIEWPORTSIZE * VIEWPORTSIZE);

	int x;
	for(x = 0; x < VIEWPORTSIZE; x++){
		int y;
		for(y = 0; y < VIEWPORTSIZE; y++){
			int i = getMapPos(minx + x, miny + y);
			if(map[i] == '%' || map[i] == '.'){
				vismap[x][y] = 1;
			}
		}
	}

	calculatelos();

	for(x = minx; x < maxx; x++){
		int y;
		for(y = miny; y < maxy; y++){
			int i = getMapPos(x, y);
			int ax = x - minx;
			int ay = y - miny;
			if(checkvismap(ax, ay)){
				continue;
			}

			int rx = ax * 8;
			int ry = ay * 8;
			if(unitmap[i] != '\0'){
				drawLetter(&tex, &font, unitmap[i], rx, ry, COLOR_LIGHTRED);
			}else if(map[i] == '#'){
				drawLetter(&tex, &font, '#', rx, ry, COLOR_DARKGREEN);
			}else if(map[i] == '.'){
				drawLetter(&tex, &font, '.', rx, ry, COLOR_DARKBROWN);
			}else if(map[i] == '*'){
				drawLetter(&tex, &font, '*', rx, ry, COLOR_LIGHTGRAY);
			}else if(map[i] == '%'){
				drawLetter(&tex, &font, '.', rx, ry, COLOR_DARKGRAY);
			}else{
				drawLetter(&tex, &font, map[i], rx, ry, COLOR_RED);
			}
		}
	}

	drawLetter(&tex, &font, '@', VIEWPORTSIZE * 4, VIEWPORTSIZE * 4, COLOR_LIGHTBLUE);
}

void render()
{
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
}

void endTurn()
{
	clearTexture(&tex, COLOR_BLACK);

	renderGui();

	renderMap();
}

void movePlayer(int x, int y)
{
	int newpos = player.pos + x + y * MAPWIDTH;
	if((map[newpos] == '.' || map[newpos] == '%') && unitmap[newpos] == '\0'){
		player.pos = newpos;
	}

	endTurn();
}

void handleKeyUp(int keycode)
{
	switch(keycode){
		case CC_KEY_LEFT:
			movePlayer(-1, 0);
			break;
		case CC_KEY_RIGHT:
			movePlayer(1, 0);
			break;
		case CC_KEY_UP:
			movePlayer(0, -1);
			break;
		case CC_KEY_DOWN:
			movePlayer(0, 1);
			break;
	}
}

int main(int argc, char **argv)
{
	initTexture(&tex, WIDTH / 2, HEIGHT / 2);

	initFont(&font, fontwidth, fontheight);
	loadFont(&font, '!', (bool*)fontdata);

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

	player.health = 100;

	generateMap();
	renderMap();

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
				handleKeyUp(ccWindowEventGet().keyCode);
			}
		}

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	return 0;
}
