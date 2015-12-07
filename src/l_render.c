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

static void renderWall(texture_t *target, const texture_t *tex, const sector_t *sect, edge_t *edge, int leftproj, int rightproj, xy_t left, xy_t right)
{
	unsigned int i;
	for(i = leftproj + screenhw; i < rightproj + screenhw; i++){
		drawLine(target, (xy_t){i, 0}, (xy_t){i, screenh}, COLOR_RED);
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
		if(transp1.y <= cam->znear || transp2.y <= cam->znear){
			continue;
		}

		//TODO fix rounding error here
		transp1.x = acos * relp1.x - asin * relp1.y;
		transp2.x = acos * relp2.x - asin * relp2.y;

		// Perspective projection
		int proj1 = transp1.x * (cam->fov / transp1.y) * screenhw + screenhw;
		int proj2 = transp2.x * (cam->fov / transp2.y) * screenhw + screenhw;

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


		drawLine(texture, (xy_t){proj1, 0}, (xy_t){proj1, 20}, COLOR_GREEN);
		drawLine(texture, (xy_t){proj2, 0}, (xy_t){proj2, 20}, COLOR_YELLOW);

		if(edge->type == PORTAL){
			edge_t *neighbor = edge->neighbor;
			if(neighbor != NULL){
				if(proj1 < camleft){
					proj1 = camleft;
				}
				if(proj2 > camright){
					proj2 = camright;
				}
				//renderSector(texture, textures, getSector(neighbor->sector), cam, left, right, neighbor);
			}
		}else if(edge->type == WALL){
			renderWall(texture, textures, sector, edge, proj1, proj2, transp2, transp1);
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

void initRender(unsigned int width, unsigned int height)
{
	screenw = width;
	screenh = height;
	screenhw = width / 2;
}

void renderFromSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam)
{
	renderSector(texture, textures, sector, cam, -((int)screenhw), (int)screenhw - 1, NULL);
}
