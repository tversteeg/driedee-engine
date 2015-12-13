#include "l_render.h"

#include "l_vector.h"
#include "l_level.h"
#include "l_colors.h"

#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define EXIT_ON_ERROR 1
#define ROUNDING_FACTOR 10.0

static unsigned int screenw = 0;
static int screenhw = 0;
static unsigned int screenh = 0;
static int screenhh = 0;

static xy_t worldToCamCoordinates(xy_t p, camera_t *cam)
{
	// Add some error so we are sure we are not dividing by zero
	p.x = cam->pos.x - p.x;
	p.y = cam->pos.z - p.y;

	v_t asin = cam->anglesin;
	v_t acos = cam->anglecos;

	long camx = (acos * p.x - asin * p.y) * ROUNDING_FACTOR;
	long camy = (asin * p.x + acos * p.y) * ROUNDING_FACTOR;

	return (xy_t){camx / ROUNDING_FACTOR, camy / ROUNDING_FACTOR};
}

static v_t projectScreenToCamAngle(int s)
{
#ifdef EXIT_ON_ERROR
	if(s < -screenhw || s >= screenhw){
		fprintf(stderr, "Error: screen projection is out of bounds: %d\n", s);
		exit(1);
	}
#endif
	return s / (v_t)screenhw;
}

static int projectCamToScreenCoordinates(xy_t p, camera_t *cam)
{
#ifdef EXIT_ON_ERROR
	if(p.y <= 0){
		fprintf(stderr, "Camera point y coordinate can't be lower then 0: %f\n", p.y);
		exit(1);
	}
#endif
	return p.x * (cam->fov / p.y) * screenhw;
}

static bool clipIntersection(xy_t p1, xy_t p2, int x, xy_t *result)
{
	v_t a = p2.y - p1.y;
	v_t b = p1.x - p2.x;
	v_t c = a * p1.x + b * p1.y;

	v_t realx = projectScreenToCamAngle(x);
	v_t delta = -a * realx - b;
	if(delta == 0){
#ifdef EXIT_ON_ERROR
		fprintf(stderr, "Error: line is parralel with the projection\n");
		exit(1);
#endif
		return false;
	}

	result->x = (-realx * c) / delta;
	result->y = -c / delta;

	return true;
}

static void renderWall(texture_t *target, const texture_t *tex, const sector_t *sect, const edge_t *edge, const camera_t *cam, 
		int leftproj, int rightproj, 
		xy_t left, xy_t right,
		bool clipleft, bool clipright)
{
	// Clip the edges when they are partially hidden
	xy_t leftfix = left;
	if(clipleft){
		if(!clipIntersection(left, right, leftproj, &leftfix)){
#ifdef EXIT_ON_ERROR
			fprintf(stderr, "Interception not possible for %d\n", leftproj);
			exit(1);
#else
			return;
#endif
		}
	}
	xy_t rightfix = right;
	if(clipright){
		if(!clipIntersection(left, right, rightproj, &rightfix)){
#ifdef EXIT_ON_ERROR
			fprintf(stderr, "Interception not possible for %d\n", rightproj);
			exit(1);
#else
			return;
#endif
		}
	}

	v_t ceilheight = sect->ceil + cam->pos.y;
	v_t floorheight = sect->floor + cam->pos.y;

	int topleft = screenhh - (ceilheight / leftfix.y) * screenhh;
	int botleft = screenhh - (floorheight / leftfix.y) * screenhh;
	int topright = screenhh - (ceilheight / rightfix.y) * screenhh;
	int botright = screenhh - (floorheight / rightfix.y) * screenhh;

	unsigned int screenwidth = rightproj - leftproj;
	v_t topslope = (topright - topleft) / (v_t)screenwidth;
	v_t botslope = (botright - botleft) / (v_t)screenwidth;

	unsigned int leftscreen = leftproj + screenhw;
	unsigned int i;
	for(i = 0; i < screenwidth; i++){
		unsigned int realx = i + leftscreen;
		xy_t top = {realx, max(0, topleft + i * topslope)};
		xy_t bot = {realx, min(screenh - 1, botleft + i * botslope)};
		drawLine(target, top, bot, COLOR_GRAY);
	}

#if DRAW_DEBUG_LEVEL >= 2
	xy_t d_p1, d_p2;

	d_p1 = (xy_t){leftproj + screenhw, topleft};
	d_p2 = (xy_t){leftproj + screenhw, botleft};
	drawLine(target + 1, d_p1, d_p2, COLOR_LIGHTGREEN);

	d_p1 = (xy_t){rightproj + screenhw, topright};
	d_p2 = (xy_t){rightproj + screenhw, botright};
	drawLine(target + 1, d_p1, d_p2, COLOR_LIGHTBLUE);
#endif

#if 0
	// Calculate the UV coordinates
	xy_t norm = {right.x - left.x, right.y - left.y};
	xy_t leftnorm = {leftfix.x - left.x, leftfix.y - left.y};
	xy_t rightnorm = {rightfix.x - right.x, rightfix.y - right.y};
	v_t leftuv = vectorProjectScalar(leftnorm, norm) / edge->uvdiv;
	v_t rightuv = vectorProjectScalar(rightnorm, norm) / edge->uvdiv;

	v_t ceilheight = sect->ceil + cam->pos.y;
	v_t floorheight = sect->floor + cam->pos.y;

	// Divide by the z value to get the distance and calculate the height with that
	v_t projtoplefty = ceilheight / leftfix.y;
	v_t projbotlefty = floorheight / leftfix.y;
	v_t projtoprighty = ceilheight / rightfix.y;
	v_t projbotrighty = floorheight / rightfix.y;

	int screentoplefty = screenhw - projtoplefty * screenhw;
	int screenbotlefty = screenhw - projbotlefty * screenhw;
	int screentoprighty = screenhw - projtoprighty * screenhw;
	int screenbotrighty = screenhw - projbotrighty * screenhw;

	unsigned int screenwidth = rightproj - leftproj;
	v_t topslope = (screentoprighty - screentoplefty) / (v_t)screenwidth;
	v_t botslope = (screenbotrighty - screenbotlefty) / (v_t)screenwidth;

	int screenleft = leftproj + screenhw;

	const texture_t *walltex = tex + edge->texture;
	const texture_t *ceiltex = tex + sect->ceiltex;
	const texture_t *floortex = tex + sect->floortex;

	unsigned int i;
	for(i = 0; i < screenwidth; i++){
		int screenx = screenleft + i;
		int top = screentoplefty + screenx * topslope;
		int bot = screenbotlefty + screenx * botslope;

		v_t xt1 = (screenwidth - i) * rightfix.y;
		v_t xt2 = i * leftfix.y;
		v_t uvfx = (leftuv * xt1 + rightuv * xt2) / (xt1 + xt2);
		unsigned int uvx = (int)(walltex->width * uvfx) % walltex->width;
		v_t uvy = walltex->height / (v_t)(bot - top);
		drawTextureSlice(target, walltex, screenx, max(top, 0), min(bot, target->height - 1), uvx, uvy);
	}
#endif
}

#define MAX_CALLS 10
static int calls = 0;
static void renderSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam, int camleft, int camright, edge_t *previous)
{
	if(++calls == MAX_CALLS){
		return;
	}

#if DRAW_DEBUG_LEVEL >= 1
	drawPixel(texture + 1, screenhw, screenhh, COLOR_GREEN);
#endif

	unsigned int i;
	for(i = 0; i < sector->nedges; i++){
		edge_t *edge = sector->edges + i;
		if(edge == previous){
			continue;
		}

		xy_t p1 = worldToCamCoordinates(sector->vertices[edge->vertex1], cam);
		xy_t p2 = worldToCamCoordinates(sector->vertices[edge->vertex2], cam);

#if DRAW_DEBUG_LEVEL >= 2
		drawPixel(texture + 1, p1.x + screenhw, screenhh - p1.y, COLOR_DARKGRAY);
		drawPixel(texture + 1, p2.x + screenhw, screenhh - p2.y, COLOR_DARKGREEN);
#endif

		// Clip portals behind zero
		v_t znear = 0.1;
		if(edge->type == WALL){
			znear = cam->znear;
		}

		// Clip everything behind the camera minimal view 
		if(p1.y < znear && p2.y < znear){
			continue;
		}

		if(p1.y > cam->zfar || p2.y > cam->zfar){
			continue;
		}

		// Use the line formula y=mx+b to project the line on the cam znear axis, so the screen projections can't ever overlap
		xy_t tempp1 = p1;
		if(p1.y < znear){
			if(p1.x != p2.x){
				tempp1.x += (znear - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
			}
			tempp1.y = znear;
		}
		if(p2.y < znear){
			if(p1.x != p2.x){
				p2.x += (znear - p2.y) * (p1.x - p2.x) / (p1.y - p2.y);
			}
			p2.y = znear;
		}
		p1 = tempp1;

		// Perspective projection
		int proj1 = projectCamToScreenCoordinates(p1, cam);
		int proj2 = projectCamToScreenCoordinates(p2, cam);

		// Don't render walls that are not facing this way
		if(proj1 > proj2){
			continue;
		}

		// Edge is out of camera bounds
		if(proj2 < -screenhw || proj1 >= screenhw){
			continue;
		}

		bool clipleft = false;
		if(proj1 <= camleft){
			proj1 = camleft;
			clipleft = true;
		}
		bool clipright = false;
		if(proj2 >= camright){
			proj2 = camright;
			clipright = true;
		}

		// Ignore the points when they are on the same pixel
		if(proj1 == proj2){
			continue;
		}

#ifdef EXIT_ON_ERROR
		if(proj1 < -screenhw || proj1 >= screenhw || proj2 < -screenhw || proj2 >= screenhw){
			fprintf(stderr, "Projection out of bounds:\t%d:%d\n", proj1, proj2);
			exit(1);
		}
#endif

#if DRAW_DEBUG_LEVEL >= 1
		pixel_t d_edgecol = COLOR_YELLOW;
#endif

		if(edge->type == PORTAL){
			edge_t *neighbor = edge->neighbor;
			if(neighbor != NULL){
				renderSector(texture, textures, getSector(neighbor->sector), cam, proj1, proj2, neighbor);
			}
#if DRAW_DEBUG_LEVEL >= 1
			d_edgecol = COLOR_BLUE;
#endif
		}else if(edge->type == WALL){
			// Clip everything behind the camera minimal view
			if((clipleft || clipright) && (p1.y < znear || p2.y < znear)){
#ifdef EXIT_ON_ERROR
				exit(1);
#else
				continue;
#endif
			}
			//renderWall(texture, textures, sector, edge, cam, proj1, proj2, p1, p2, clipleft, clipright);

#if DRAW_DEBUG_LEVEL >= 1
			if(clipleft){
				clipIntersection(p1, p2, proj1, &p1);
			}
			if(clipright){
				clipIntersection(p1, p2, proj2, &p2);
			}
#endif
		}

#if DRAW_DEBUG_LEVEL >= 1
		xy_t d_p1, d_p2;

#if DRAW_DEBUG_LEVEL >= 3
		d_p1 = (xy_t){screenhw, screenhh};
		d_p2 = (xy_t){projectScreenToCamAngle(proj1) * screenhh + screenhw, 0};
		drawLine(texture + 1, d_p1, d_p2, COLOR_DARKGREEN);

		d_p2 = (xy_t){projectScreenToCamAngle(proj2) * screenhh + screenhw, 0};
		drawLine(texture + 1, d_p1, d_p2, COLOR_DARKRED);
#endif

		d_p1 = (xy_t){p1.x + screenhw, -p1.y + screenhh};
		d_p2 = (xy_t){p2.x + screenhw, -p2.y + screenhh};
		drawLine(texture + 1, d_p1, d_p2, d_edgecol);
#endif
	}
}

void setCameraRotation(camera_t *cam, v_t angle)
{
	cam->angle = angle;
	cam->anglesin = sinf(angle);
	cam->anglecos = cosf(angle);
}

void calculateViewport(camera_t *cam, xy_t right)
{
	xy_t camunit = vectorUnit(right);
	cam->fov = camunit.x * camunit.y * 2.0;
}

void initRender(unsigned int width, unsigned int height, camera_t *cam)
{
	screenw = width;
	screenh = height;
	screenhw = width / 2;
	screenhh = height / 2;
}

void renderFromSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam)
{
	calls = 0;
	renderSector(texture, textures, sector, cam, -screenhw, screenhw - 1, NULL);
}
