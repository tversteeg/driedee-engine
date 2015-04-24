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

#include "l_draw.h"
#include "l_sector.h"
#include "l_level.h"
#include "l_render.h"
#include "l_colors.h"
#include "l_png.h"

#define WIDTH 1280
#define HWIDTH (WIDTH / 2)
#define HEIGHT 800
#define HHEIGHT (HEIGHT / 2)

#define PLAYER_SPEED 0.2
#define PLAYER_JUMP -0.5
#define PLAYER_JUMP_WOBBLE -0.2
#define PLAYER_FRICTION 0.8
#define PLAYER_GRAVITY 0.02

#define USE_MOUSE

struct player {
	camera_t cam;
	xyz_t pos, vel;
	double height, radius;
	sector_t *sector;
} player;

typedef struct {
	bool active;
	sprite_t *sprite;
	sector_t *sect;
	xy_t vel;
} bullet_t;

GLuint texture;
texture_t tex;

texture_t *gametextures;
size_t ngametextures;

bool isshooting;

bullet_t *bullets = NULL;
int nbullets = 0, bulletdelay = 0;

void handleGame()
{
	bulletdelay++;

	if(isshooting && bulletdelay >= 10){
		bullet_t *bullet = NULL;
		int i;
		for(i = 0; i < nbullets; i++){
			if(!bullets[i].active){
				bullet = bullets + i;
				break;
			}
		}

		if(bullet == NULL){
			bullets = (bullet_t*)realloc(bullets, ++nbullets * sizeof(bullet_t));
			bullet = bullets + nbullets - 1;
		}

		double playerangle = -player.cam.angle - M_PI / 2;
		bullet->vel.x = cos(playerangle) * 5;
		bullet->vel.y = sin(playerangle) * 5;

		xyz_t bulletpos = (xyz_t){player.pos.x + bullet->vel.x * 5.0, -player.pos.y - 5, player.pos.z + bullet->vel.y * 5.0};
		bullet->sprite = spawnSprite(player.sector, bulletpos, (xy_t){50, 50}, 4);
		bullet->sect = player.sector;
		bullet->active = true;

		bulletdelay = 0;
	}

	int i;
	for(i = 0; i < nbullets; i++){
		bullet_t *bullet = bullets + i;
		if(!bullet->active){
			continue;
		}
		xy_t newpos;
		newpos.x = bullet->sprite->pos.x + bullet->vel.x;
		newpos.y = bullet->sprite->pos.z + bullet->vel.y;
		
		if(tryMoveSprite(bullet->sect, bullet->sprite, newpos) == NULL){
			destroySprite(bullet->sect, bullet->sprite);
			bullet->active = false;
		}
	}
}

void render()
{
	renderFromSector(&tex, gametextures, player.sector, &player.cam);

	texture_t *gun = gametextures + 3;
	drawTextureScaled(&tex, gun, HWIDTH - gun->width * 1.5 - 40, HEIGHT - gun->height * 3, (xy_t){3, 3});

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

void movePlayer(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed, bool spacePressed)
{
	if(spacePressed){
		if(player.pos.y > -1){
			player.vel.y = PLAYER_JUMP;
			player.pos.y = -1;
		}
	}
	if(upPressed){
		player.vel.x += cos(player.cam.angle + M_PI / 2) * PLAYER_SPEED;
		player.vel.z -= sin(player.cam.angle + M_PI / 2) * PLAYER_SPEED;

		if(player.pos.y >= 0){
			player.vel.y = PLAYER_JUMP_WOBBLE;
			player.pos.y = -0.01;
		}
	}
	if(downPressed){
		player.vel.x += cos(player.cam.angle - M_PI / 2) * PLAYER_SPEED;
		player.vel.z -= sin(player.cam.angle - M_PI / 2) * PLAYER_SPEED;
	}
	if(leftPressed){
#ifdef USE_MOUSE
		player.vel.x += cos(player.cam.angle) * PLAYER_SPEED / 2;
		player.vel.z -= sin(player.cam.angle) * PLAYER_SPEED / 2;
#else
		player.cam.angle -= 0.035f;
#endif
	}
	if(rightPressed){
#ifdef USE_MOUSE
		player.vel.x += cos(player.cam.angle - M_PI) * PLAYER_SPEED / 2;
		player.vel.z -= sin(player.cam.angle - M_PI) * PLAYER_SPEED / 2;
#else
		player.cam.angle += 0.035f;
#endif
	}

	if(player.vel.x > V_ERROR || player.vel.x < -V_ERROR || player.vel.y > V_ERROR || player.vel.y < -V_ERROR){
		unsigned int i;
		sector_t *next = NULL;
		for(i = 0; i < player.sector->nedges; i++){
			edge_t *edge = player.sector->edges + i;
			if(edge->type != PORTAL){
				continue;
			}

			xy_t playerpos = {player.pos.x, player.pos.z};
			xy_t playerposnext = {player.pos.x + player.vel.x, player.pos.z + player.vel.z};
			xy_t edge1 = player.sector->vertices[edge->vertex1];
			xy_t edge2 = player.sector->vertices[edge->vertex2];
			xy_t result;
			if(segmentSegmentIntersect(playerpos, playerposnext, edge1, edge2, &result)){
				next = edge->neighbor->sector;
			}
		}

		if(next != NULL){
			player.sector = next;
		}

		player.pos.x += player.vel.x;
		player.pos.z += player.vel.z;
		player.vel.x *= PLAYER_FRICTION;
		player.vel.z *= PLAYER_FRICTION;
	}
	if(player.pos.y < -V_ERROR){
		player.pos.y += player.vel.y;
		player.vel.y += PLAYER_GRAVITY;
	}else{
		player.pos.y = 0;
	}

	player.cam.pos = player.pos;
	player.cam.pos.y += player.height;

#ifdef USE_MOUSE
	player.cam.angle += (ccWindowGetMouse().x - HWIDTH) / 1000.0f;
	ccWindowMouseSetPosition((ccPoint){HWIDTH, HHEIGHT});
#endif
}

int main(int argc, char **argv)
{
	bool loop, upPressed, downPressed, leftPressed, rightPressed, spacePressed;

	sectorInitialize();

	initTexture(&tex, WIDTH, HEIGHT);

	ngametextures = 5;
	gametextures = (texture_t*)malloc(ngametextures * sizeof(texture_t));

	unsigned int width, height;
	getSizePng("wall1.png", &width, &height);
	initTexture(gametextures, width, height);
	loadPng(gametextures, "wall1.png");
	
	getSizePng("wall2.png", &width, &height);
	initTexture(gametextures + 1, width, height);
	loadPng(gametextures + 1, "wall2.png");

	getSizePng("skeleton.png", &width, &height);
	initTexture(gametextures + 2, width, height);
	loadPng(gametextures + 2, "skeleton.png");
	
	getSizePng("gun2.png", &width, &height);
	initTexture(gametextures + 3, width, height);
	loadPng(gametextures + 3, "gun2.png");

	getSizePng("bullet.png", &width, &height);
	initTexture(gametextures + 4, width, height);
	loadPng(gametextures + 4, "bullet.png");

	loadLevel(argv[1]);
	player.sector = getSector(0);
	player.pos.x = player.sector->vertices[0].x + 5;
	player.pos.z = player.sector->vertices[0].y;
	player.pos.y = player.vel.x = player.vel.y = player.vel.z = player.cam.angle = 0;
	player.cam.znear = 1;
	player.cam.zfar = 200;
	calculateViewport(&player.cam, (xy_t){1, 1});
	player.cam.pos = player.pos;

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
	isshooting = spacePressed = leftPressed = rightPressed = upPressed = downPressed = false;
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
			}else if(ccWindowEventGet().type == CC_EVENT_MOUSE_DOWN){
				isshooting = true;
			}else if(ccWindowEventGet().type == CC_EVENT_MOUSE_UP){
				isshooting = false;
			}
		}

		movePlayer(upPressed, downPressed, leftPressed, rightPressed, spacePressed);

		handleGame();

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
