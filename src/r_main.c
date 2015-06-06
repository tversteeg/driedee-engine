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

typedef enum { ENEMY_RAT, ENEMY_ARCHER } enemytype_t;

typedef struct {
	enemytype_t type;
	int pos;
	int health;
} enemy_t;

typedef struct _itemtype_t {
	struct _itemtype_t *parent;
	char name[20];
} itemtype_t;

typedef struct {
	itemtype_t *type;
	bool onplayer;
	int pos, amount;
} item_t;

struct {
	int pos;
	int health, coins;
	item_t *items;
	int nitems;
} player;

itemtype_t weapontype = {NULL, "Weapon"};
itemtype_t swordtype = {&weapontype, "Sword"};
itemtype_t speartype = {&weapontype, "Spear"};

itemtype_t potiontype = {NULL, "Potion"};
itemtype_t healthtype = {&potiontype, "Health Potion"};
itemtype_t manatype = {&potiontype, "Mana Potion"};

itemtype_t *droptypes[] = {&healthtype, &manatype, &swordtype, &speartype};

char map[MAPSIZE];
char vismap[MAPSIZE];
char viewportmap[VIEWPORTSIZE][VIEWPORTSIZE];
enemy_t *enemies;
int nenemies;
item_t *items;
int nitems;

GLuint texture;
texture_t tex;
font_t font;

void spawnEnemy(int pos, enemytype_t type)
{
	enemies = (enemy_t*)realloc(enemies, ++nenemies * sizeof(enemy_t));

	enemy_t *enemy = enemies + nenemies - 1;
	enemy->pos = pos;
	enemy->type = type;

	switch(type){
		case ENEMY_RAT:
			enemy->health = 10;
			break;
		case ENEMY_ARCHER:
			enemy->health = 100;
			break;
	}
}

void spawnItem(int pos, int typeindex)
{
	items = (item_t*)realloc(items, ++nitems * sizeof(item_t));

	item_t *item = items + nitems - 1;
	item->pos = pos;
	item->type = droptypes[typeindex];
	item->amount = 1;
}

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

void generateMap()
{
	nenemies = 0;
	enemies = NULL;

	nitems = 0;
	items = NULL;

	srand(time(NULL));

	int i;
	for(i = 0; i < MAPSIZE; i++){
		map[i] = vismap[i] = '\0';
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
			spawnEnemy(i, (enemytype_t)(rand() % 2));
		}
	}
	
	for(i = 0; i < MAPSIZE; i++){
		if(map[i] == '%' && rand() % 20 == 0){
			spawnItem(i, rand() % (sizeof(droptypes) / sizeof(droptypes[0])));
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

	int i;
	for(i = 0; i < player.nitems; i++){
		char *itemstr = (char*)malloc(30);
		snprintf(itemstr, 30, "Item: %d %s\n", player.items[i].amount, player.items[i].type->name);
		drawString(&tex, &font, itemstr, VIEWPORTSIZE * 8 + 8, 32 + i * 8, COLOR_YELLOW);
		free(itemstr);
	}
}

bool checkViewport(int x, int y)
{
	if(viewportmap[x][y] != 0){
		return false;
	}
	if(x > 0){
		if(viewportmap[x - 1][y] != 0){
			return false;
		}
	}
	if(x < VIEWPORTSIZE - 1){
		if(viewportmap[x + 1][y] != 0){
			return false;
		}
	}
	if(y > 0){
		if(viewportmap[x][y - 1] != 0){
			return false;
		}
	}
	if(y < VIEWPORTSIZE - 1){
		if(viewportmap[x][y + 1] != 0){
			return false;
		}
	}

	return true;
}

void setLOSViewport(int x, int y)
{
	int x2 = VIEWPORTSIZE / 2;
	int y2 = VIEWPORTSIZE / 2;
	int dx = abs(x - x2);
	int dy = abs(y - y2);

	int sx = x2 < x ? 1 : -1;
	int sy = y2 < y ? 1 : -1;

	int err = (dx > dy ? dx : -dy) / 2;
	bool ocluded = false;
	while(true){
		if(viewportmap[x2][y2] <= 0){
			ocluded = true;
		}
		if(ocluded){
			viewportmap[x][y] = 0;
			return;
		}
		if(x == x2 && y == y2){
			return;
		}
		int err2 = err;
		if(err2 > -dx){
			err -= dy;
			x2 += sx;
		}
		if(err2 < dy){
			err += dx;
			y2 += sy;
		}
	}
}

void calculateLOS()
{
	int x;
	for(x = 0; x < VIEWPORTSIZE; x++){
		int y;
		for(y = 0; y < VIEWPORTSIZE; y++){
			setLOSViewport(x, y);
		}
	}
}

void renderMap()
{
	int px = getMapX(player.pos);
	int py = getMapY(player.pos);
	int minx = px - VIEWPORTSIZE / 2;
	int maxx = minx + VIEWPORTSIZE;
	int miny = py - VIEWPORTSIZE / 2;
	int maxy = miny + VIEWPORTSIZE;

	memset(&viewportmap, 0, VIEWPORTSIZE * VIEWPORTSIZE);

	int x;
	for(x = 0; x < VIEWPORTSIZE; x++){
		int y;
		for(y = 0; y < VIEWPORTSIZE; y++){
			int i = getMapPos(minx + x, miny + y);
			if(map[i] == '%' || map[i] == '.'){
				viewportmap[x][y] = 1;
			}
		}
	}

	calculateLOS();

	for(x = minx; x < maxx; x++){
		int y;
		for(y = miny; y < maxy; y++){
			int i = getMapPos(x, y);
			int ax = x - minx;
			int ay = y - miny;
			if(!checkViewport(ax, ay)){
				int rx = ax * 8;
				int ry = ay * 8;
				if(map[i] == '#'){
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

				vismap[i] = 1;
			}else if(vismap[i] > 0){
				int rx = ax * 8;
				int ry = ay * 8;
				if(map[i] == '#'){
					drawLetter(&tex, &font, '#', rx, ry, COLOR_DARKGRAY);
				}else if(map[i] == '*'){
					drawLetter(&tex, &font, '*', rx, ry, COLOR_DARKGRAY);
				}
			}
		}
	}

	int i;
	for(i = 0; i < nenemies; i++){
		enemy_t enemy = enemies[i];
		int ex = getMapX(enemy.pos) - minx;
		int ey = getMapY(enemy.pos) - miny;
		if(ex >= 0 && ex < VIEWPORTSIZE && ey >= 0 && ey < VIEWPORTSIZE && !checkViewport(ex, ey)){
			char enemychar = '\0';
			pixel_t color;
			switch(enemy.type){
				case ENEMY_RAT:
					enemychar = 'R';
					color = COLOR_LIGHTBROWN;
					break;
				case ENEMY_ARCHER:
					enemychar = 'A';
					color = COLOR_GREEN;
					break;
			}
			drawLetter(&tex, &font, enemychar, ex * 8, ey * 8, color);
		}
	}

	for(i = 0; i < nitems; i++){
		if(items + i == NULL){
			continue;
		}
		item_t item = items[i];
		int ex = getMapX(item.pos) - minx;
		int ey = getMapY(item.pos) - miny;
		if(ex >= 0 && ex < VIEWPORTSIZE && ey >= 0 && ey < VIEWPORTSIZE && !checkViewport(ex, ey)){
			drawLetter(&tex, &font, 'C', ex * 8, ey * 8, COLOR_BROWN);
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
	int i, newpos = player.pos + x + y * MAPWIDTH;
	for(i = 0; i < nenemies; i++){
		if(enemies[i].pos == newpos){
			return;
		}
	}
	if(map[newpos] != '.' && map[newpos] != '%'){
		return;
	}
		
	player.pos = newpos;

	for(i = 0; i < nitems; i++){
		if(player.pos == items[i].pos){
			int j;
			bool hasitem = false;
			for(j = 0; j < player.nitems; j++){
				if(player.items[j].type == items[i].type){
					hasitem = true;
					player.items[j].amount += items[i].amount;
					break;
				}
			}
			if(!hasitem){
				player.items = (item_t*)realloc(player.items, ++player.nitems * sizeof(item_t));
				memcpy(player.items + player.nitems - 1, items + i, sizeof(item_t));
			}
			
			memmove(items + i, items + i + 1, (--nitems - i) * sizeof(item_t));
			break;
		}
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
	player.nitems = 0;

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
