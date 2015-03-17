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
#include "l_colors.h"

#define WIDTH 800
#define HEIGHT 600
#define HWIDTH (WIDTH / 2)
#define HHEIGHT (HEIGHT / 2)

#define ASPECT (WIDTH / (double)HEIGHT)

#define PLAYER_SPEED 0.5f
#define PLAYER_JUMP -1.8f
#define PLAYER_FRICTION 0.8f
#define PLAYER_GRAVITY 0.1f

//#define USE_MOUSE

struct player {
	xyz_t pos, vel;
	double angle, fov, yaw, height, radius;
	sector_t *sector;
} player;

GLuint texture;
texture_t tex;

double yLookup[HHEIGHT];

void clipPointToCamera(xy_t camleft, xy_t camright, xy_t *p1, xy_t p2)
{
	if(p1->y < 0){
		p1->x += -p1->y * (p2.x - p1->x) / (p2.y - p1->y);
		p1->y = 0;
	}

	xy_t cam;
	if(vectorIsLeft(*p1, (xy_t){0, 0}, camleft)){
		cam = camleft;
	}else{
		cam = camright;
	}

	lineSegmentIntersect((xy_t){0, 0}, cam, *p1, p2, p1);
}

void populateLookupTables()
{
	unsigned int i;
	for(i = 1; i < HHEIGHT; i++){
		yLookup[i] = HEIGHT / (double)(i * 2.0f);
	}
}

void renderWall(xy_t left, xy_t right, double camlen, double top, double bottom, double above, double beneath)
{
	if(left.y <= 1 || right.y <= 1){
		return;
	}

	// Near plane is 1, find x position on the plane
	double projleftx = (left.x / left.y) * player.fov;
	double projrightx = (right.x / right.y) * player.fov;

	// Convert to screen coordinates
	int screenleftx = HWIDTH + projleftx * HWIDTH;
	int screenrightx = HWIDTH + projrightx * HWIDTH;
	if(screenleftx == screenrightx){
		return;
	}

	// Divide by the y value to get the distance and use that to calculate the height
	double eyeheight = player.pos.z - player.height;
	double projtoplefty = (top + eyeheight) / left.y;
	double projbotlefty = (bottom + eyeheight) / left.y;
	double projtoprighty = (top + eyeheight) / right.y;
	double projbotrighty = (bottom + eyeheight) / right.y;

	int screentoplefty = HHEIGHT - projtoplefty * HHEIGHT;
	int screenbotlefty = HHEIGHT - projbotlefty * HHEIGHT;
	int screentoprighty = HHEIGHT - projtoprighty * HHEIGHT;
	int screenbotrighty = HHEIGHT - projbotrighty * HHEIGHT;

	drawLine(&tex, (xy_t){(double)screenleftx, (double)screentoplefty}, (xy_t){(double)screenrightx, (double)screentoprighty}, COLOR_WHITE);
	drawLine(&tex, (xy_t){(double)screenleftx, (double)screenbotlefty}, (xy_t){(double)screenrightx, (double)screenbotrighty}, COLOR_WHITE);

	drawLine(&tex, (xy_t){(double)screenleftx, (double)screentoplefty}, (xy_t){(double)screenleftx, (double)screenbotlefty}, COLOR_WHITE);
	drawLine(&tex, (xy_t){(double)screenrightx, (double)screentoprighty}, (xy_t){(double)screenrightx, (double)screenbotrighty}, COLOR_WHITE);
}

void renderSector(sector_t *sector, xy_t campos, xy_t camleft, xy_t camright, double camlen, edge_t *ignore)
{
	double sina = sin(player.angle);
	double cosa = cos(player.angle);
	xy_t camleftnorm = vectorUnit(camleft);
	xy_t camrightnorm = vectorUnit(camright);

	unsigned int i;
	for(i = 0; i < sector->nedges; i++){
		edge_t *edge = sector->edges + i;
		if(edge == ignore){
			continue;
		}

		xy_t p1 = sector->vertices[edge->vertex1];
		xy_t p2 = sector->vertices[edge->vertex2];

		xy_t v1 = {campos.x - p1.x, campos.y - p1.y};
		xy_t v2 = {campos.x - p2.x, campos.y - p2.y};

		// 2D transformation matrix for rotations
		xy_t tv1 = {.y = sina * v1.x + cosa * v1.y};
		xy_t tv2 = {.y = sina * v2.x + cosa * v2.y};

		// Clip everything behind the player
		if(tv1.y <= 0 && tv2.y <= 0){
			continue;
		}

		tv1.x = cosa * v1.x - sina * v1.y;
		tv2.x = cosa * v2.x - sina * v2.y;

		// Clip everything outside of the field of view
		xy_t uv1 = vectorUnit(tv1);
		xy_t uv2 = vectorUnit(tv2);
		bool notbetween1 = !vectorIsBetween(uv1, camleftnorm, camrightnorm);
		bool notbetween2 = !vectorIsBetween(uv2, camleftnorm, camrightnorm);
		if(notbetween1 && notbetween2){
			// Remove them if they both lie on the same side
			if(vectorIsLeft(tv1, (xy_t){0, 0}, camleftnorm) && vectorIsLeft(tv2, (xy_t){0, 0}, camleftnorm)){
				continue;
			}else if(!vectorIsLeft(tv1, (xy_t){0, 0}, camrightnorm) && !vectorIsLeft(tv2, (xy_t){0, 0}, camrightnorm)){
				continue;
			}else if(tv1.y - ((tv2.y - tv1.y) / (tv2.x - tv1.x)) * tv1.x <= 0){
				// Use the function y = ax + b to determine if the line is above or under the player and clip if it's under
				continue;
			}
		}

		v1.x = tv1.x;
		v1.y = tv1.y;
		if(notbetween1 && !vectorIsEqual(tv1, camleft) && !vectorIsEqual(tv1, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &v1, tv2);
		}
		if(notbetween2 && !vectorIsEqual(tv2, camleft) && !vectorIsEqual(tv2, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &tv2, tv1);
		}
		tv1.x = v1.x;
		tv1.y = v1.y;

		double cross = vectorCrossProduct(tv1, tv2);
		if(cross > 0){
			xy_t temp = tv1;
			tv1 = tv2;
			tv2 = temp;
		}

		if(edge->type == PORTAL && edge->neighbor != NULL){
			printf("P1 %p\nP2 %p\n", edge->neighbor, edge->neighbor->sector);
			renderSector(edge->neighbor->sector, campos, tv1, tv2, camlen, edge->neighbor);
			printf("Edges %d\n", edge->neighbor->sector->nedges);
		}else if(edge->type == WALL){
			renderWall(tv1, tv2, camlen, 10, 0, HEIGHT, 0);
		}
	}
}

void renderScene()
{
	xy_t camleft = {-200, 200};
	xy_t camright = {200, 200};

	xy_t camunit = vectorUnit(camright);
	player.fov = (camunit.x * camunit.y) * 2;

	renderSector(player.sector, (xy_t){player.pos.x, player.pos.y}, camleft, camright, 1000, NULL);
}

void render()
{
	renderScene();

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
	if(upPressed){
		player.vel.x += cos(player.angle + M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle + M_PI / 2) * PLAYER_SPEED;
	}
	if(downPressed){
		player.vel.x += cos(player.angle - M_PI / 2) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle - M_PI / 2) * PLAYER_SPEED;
	}
	if(leftPressed){
#ifdef USE_MOUSE
		player.vel.x += cos(player.angle) * PLAYER_SPEED;
		player.vel.y -= sin(player.angle) * PLAYER_SPEED;
#else
		player.angle -= 0.035f;
#endif
	}
	if(rightPressed){
#ifdef USE_MOUSE
		player.vel.x += cos(player.angle - M_PI) * PLAYER_SPEED / 2;
		player.vel.y -= sin(player.angle - M_PI) * PLAYER_SPEED / 2;
#else
		player.angle += 0.035f;
#endif
	}
	if(spacePressed){
		if(player.pos.z >= 0){
			player.vel.z = PLAYER_JUMP;
			player.pos.z = -0.1f;
		}
	}

	if(player.vel.x > V_ERROR || player.vel.x < -V_ERROR || player.vel.y > V_ERROR || player.vel.y < -V_ERROR){
		player.pos.x += player.vel.x;
		player.pos.y += player.vel.y;
		player.vel.x *= PLAYER_FRICTION;
		player.vel.y *= PLAYER_FRICTION;
	}
	if(player.pos.z < -V_ERROR){
		player.pos.z += player.vel.z;
		player.vel.z += PLAYER_GRAVITY;
	}else{
		player.pos.z = 0;
	}

#ifdef USE_MOUSE
	player.angle += (ccWindowGetMouse().x - HWIDTH) / 1000.0f;
	player.yaw += (ccWindowGetMouse().y - HHEIGHT) / 1000.0f;
	ccWindowMouseSetPosition((ccPoint){HWIDTH, HHEIGHT});
#endif
}

int main(int argc, char **argv)
{
	bool loop, upPressed, downPressed, leftPressed, rightPressed, spacePressed;

	sectorInitialize();

	initTexture(&tex, WIDTH, HEIGHT);

	loadLevel(argv[1]);
	player.sector = getFirstSector();
	player.pos.x = player.sector->vertices[0].x;
	player.pos.y = player.sector->vertices[0].y;

	populateLookupTables();

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
	spacePressed = leftPressed = rightPressed = upPressed = downPressed = false;
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
			}
		}

		movePlayer(upPressed, downPressed, leftPressed, rightPressed, spacePressed);

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
