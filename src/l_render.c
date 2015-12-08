#include "l_render.h"

#include "l_vector.h"
#include "l_level.h"
#include "l_colors.h"

#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static unsigned int screenw = 0;
static unsigned int screenhw = 0;
static unsigned int screenh = 0;
static v_t *screenlookup = NULL;

static bool clipIntersection(xy_t p1, xy_t p2, int x, xy_t *result)
{
	v_t a = p2.y - p1.y;
	v_t b = p1.x - p2.x;
	v_t c = a * p1.x + b * p1.y;

	v_t realx = screenlookup[x + screenhw];
	v_t delta = -a * realx - b;
	if(delta == 0){
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
		if(!clipIntersection(left, right, leftproj / cam->fov, &leftfix)){
			exit(1);
		}
	}
	xy_t rightfix = right;
	if(clipright){
		if(!clipIntersection(left, right, rightproj / cam->fov, &rightfix)){
			exit(1);
		}
	}

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
		int top = max(screentoplefty + screenx * topslope, 0);
		int bot = min(screenbotlefty + screenx * botslope, target->height - 1);
		
		v_t xt1 = (screenwidth - i) * rightfix.y;
		v_t xt2 = i * leftfix.y;
		v_t uvx = (leftuv * xt1 + rightuv * xt2) / (xt1 + xt2);
		drawTextureSlice(target, walltex, screenx, top, bot, uvx);
	}
}

static void renderSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam, int camleft, int camright, edge_t *previous)
{
	if(sector == NULL){
		fprintf(stderr, "Edge's sector is undefined\n");
		exit(1);
	}

	v_t asin = cam->anglesin;
	v_t acos = cam->anglecos;

	unsigned int i;
	for(i = 0; i < sector->nedges; i++){
		edge_t *edge = sector->edges + i;
		if(edge == previous){
			continue;
		}

		// Get the position of the vertices
		xy_t p1 = sector->vertices[edge->vertex1];
		xy_t p2 = sector->vertices[edge->vertex2];

		// Get the position of the vertices in relation to the camera
		xy_t relp1 = {cam->pos.x - p1.x, cam->pos.z - p1.y};
		xy_t relp2 = {cam->pos.x - p2.x, cam->pos.z - p2.y};

		// Rotate the vertices according to the angle of the camera
		xy_t transp1 = {.y = asin * relp1.x + acos * relp1.y};
		xy_t transp2 = {.y = asin * relp2.x + acos * relp2.y};

		// Clip everything behind the camera minimal view
		if(transp1.y <= cam->znear && transp2.y <= cam->znear){
			continue;
		}

		//TODO fix rounding error here
		transp1.x = acos * relp1.x - asin * relp1.y;
		transp2.x = acos * relp2.x - asin * relp2.y;

		// Perspective projection
		int proj1 = transp1.x * (cam->fov / transp1.y) * screenhw;
		int proj2 = transp2.x * (cam->fov / transp2.y) * screenhw;

		if(proj1 == proj2){
			continue;
		}else if(proj1 > proj2){
			int tempproj = proj1;
			proj1 = proj2;
			proj2 = tempproj;
		}

		// Clip the edge if they are both outside of the screen
		if(proj1 > camright || proj2 < camleft){
			continue;
		}

		bool clipleft = false;
		if(proj1 < camleft){
			proj1 = camleft;
			clipleft = true;
		}
		bool clipright = false;
		if(proj2 > camright){
			proj2 = camright;
			clipright = true;
		}

		if(edge->type == PORTAL){
			edge_t *neighbor = edge->neighbor;
			if(neighbor != NULL){
				renderSector(texture, textures, getSector(neighbor->sector), cam, proj1, proj2, neighbor);
			}
		}else if(edge->type == WALL){
			renderWall(texture, textures, sector, edge, cam,proj1, proj2, transp2, transp1, clipleft, clipright);
		}
	}
}

void setCameraRotation(camera_t *cam, v_t angle)
{
	cam->angle = angle;
	cam->anglesin = sin(angle);
	cam->anglecos = cos(angle);
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

	screenlookup = (v_t*)malloc(width * sizeof(v_t));
	unsigned int i;
	for(i = 0; i < width; i++){
		screenlookup[i] = (i - screenhw) * cam->fov;
	}
}

void renderFromSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam)
{
	renderSector(texture, textures, sector, cam, -((int)screenhw), (int)screenhw - 1, NULL);
}
