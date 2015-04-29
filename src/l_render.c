#include "l_render.h"

#include "l_vector.h"
#include "l_level.h"

#include <math.h>

void clipPointToCamera(xy_t camleft, xy_t camright, xy_t *p1, xy_t p2)
{
	if(p1->y < 0){
		p1->x += -p1->y * (p2.x - p1->x) / (p2.y - p1->y);
		p1->y = 0;
	}

	xy_t cam;
	if(vectorIsLeft(*p1, XY_ZERO, camleft)){
		cam = camleft;
	}else{
		cam = camright;
	}

	lineSegmentIntersect(XY_ZERO, cam, *p1, p2, p1);
}

void renderWall(texture_t *target, const texture_t *textures, const sector_t *sect, const camera_t *cam, edge_t *edge, xy_t left, xy_t right, double leftuv, double rightuv)
{
	// Find x position on the near plane
	//TODO change near plane from 1 to cam->znear
	double projleftx = (left.x / left.y) * cam->fov;
	double projrightx = (right.x / right.y) * cam->fov;

	int halfwidth = target->width >> 1;
	int halfheight = target->height >> 1;

	// Convert to screen coordinates
	int screenleftx = halfwidth + projleftx * halfwidth;
	int screenrightx = halfwidth + projrightx * halfwidth;
	if(screenleftx == screenrightx){
		return;
	}

	// Divide by the z value to get the distance and calculate the height with that
	double projtoplefty = (sect->ceil + cam->pos.y) / left.y;
	double projbotlefty = (sect->floor + cam->pos.y) / left.y;
	double projtoprighty = (sect->ceil + cam->pos.y) / right.y;
	double projbotrighty = (sect->floor + cam->pos.y) / right.y;

	int screentoplefty = halfheight - projtoplefty * halfheight;
	int screenbotlefty = halfheight - projbotlefty * halfheight;
	int screentoprighty = halfheight - projtoprighty * halfheight;
	int screenbotrighty = halfheight - projbotrighty * halfheight;

	int screenwidth = screenrightx - screenleftx;
	double slopetop = (screentoprighty - screentoplefty) / (double)screenwidth;
	double slopebot = (screenbotrighty - screenbotlefty) / (double)screenwidth;

	int x;
	for(x = 0; x < screenwidth; x++){
		int top = screentoplefty + x * slopetop;
		int bot = screenbotlefty + x * slopebot;
		// Affine transformation
		/*
			 double uvdiff = (rightuv - leftuv) / (double)screenwidth;
			 drawTextureSlice(target, textures, screenleftx + x, top, bot - top, leftuv + (x * uvdiff));
			 */

		// Perspective transformation
		/* Naive method
			 double alpha = x / (double)screenwidth;
			 double uvx = ((1 - alpha) * (leftuv / left.y) + alpha * (rightuv / right.y)) / ((1 - alpha) / left.y + alpha / right.y);
			 */
		double xt1 = (screenwidth - x) * right.y;
		double xt2 = x * left.y;
		double uvx = (leftuv * xt1 + rightuv * xt2) / (xt1 + xt2);
		drawTextureSlice(target, textures, screenleftx + x, top, bot - top, uvx);
	}
}

void renderSprite(texture_t *target, const texture_t *sheet, const camera_t *cam, sprite_t *sprite, xy_t pos)
{
	xy_t proj = {(pos.x / pos.y) * cam->fov, (sprite->pos.y + cam->pos.y) / pos.y};

	int halfwidth = target->width >> 1;
	int halfheight = target->height >> 1;

	xy_t scale = {sprite->scale.x * (1.0 / pos.y), sprite->scale.y * (1.0 / pos.y)};

	int screenx = halfwidth + proj.x * halfwidth - (sheet->width >> 1) * scale.x;
	int screeny = halfheight - proj.y * halfheight - sheet->height * scale.y;

	drawTextureScaled(target, sheet, screenx, screeny, scale);
}

static void renderSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam, xy_t camleft, xy_t camright, edge_t *previous)
{
	double anglesin = sin(cam->angle);
	double anglecos = cos(cam->angle);

	xy_t camleftnorm = vectorUnit(camleft);
	xy_t camrightnorm = vectorUnit(camright);

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
		xy_t transp1 = {.y = anglesin * relp1.x + anglecos * relp1.y};
		xy_t transp2 = {.y = anglesin * relp2.x + anglecos * relp2.y};

		// Clip everything behind the camera
		if(transp1.y <= 0 && transp2.y <= 0){
			continue;
		}

		//TODO fix rounding error here
		transp1.x = anglecos * relp1.x - anglesin * relp1.y;
		transp2.x = anglecos * relp2.x - anglesin * relp2.y;

		xy_t unit1 = vectorUnit(transp1);
		xy_t unit2 = vectorUnit(transp2);
		bool isnotinview1 = !vectorIsBetween(unit1, camleftnorm, camrightnorm);
		bool isnotinview2 = !vectorIsBetween(unit2, camleftnorm, camrightnorm);
		if(isnotinview1 && isnotinview2){
			// Clip the edge when it lies next to the camera
			if(vectorIsLeft(transp1, XY_ZERO, camleftnorm) &&
					vectorIsLeft(transp2, XY_ZERO, camleftnorm)){
				continue;
			}else if(!vectorIsLeft(transp1, XY_ZERO, camrightnorm) &&
					!vectorIsLeft(transp2, XY_ZERO, camrightnorm)){
				continue;
			}else if(transp1.y - ((transp2.y - transp1.y) / (transp2.x - transp1.x)) * transp1.x < V_ERROR){
				// Clip the edge when the line 'y = ax + b' is above the camera
				continue;
			}
		}

		xy_t camedge1 = transp1;
		if(isnotinview1 && !vectorIsEqual(transp1, camleft) && !vectorIsEqual(transp1, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &camedge1, transp2);
		}
		xy_t camedge2 = transp2;
		if(isnotinview2 && !vectorIsEqual(transp2, camleft) && !vectorIsEqual(transp2, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &camedge2, transp1);
		}

		double cross = vectorCrossProduct(camedge1, camedge2);
		if(cross > 0){
			xy_t temp = transp1;
			transp1 = transp2;
			transp2 = temp;

			temp = camedge1;
			camedge1 = camedge2;
			camedge2 = temp;
		}

		if(edge->type == PORTAL){
			edge_t *neighbor = edge->neighbor;
			if(neighbor != NULL){
				renderSector(texture, textures, neighbor->sector, cam, camedge1, camedge2, neighbor);
			}
		}else if(edge->type == WALL){
			xy_t norm = {transp2.x - transp1.x, transp2.y - transp1.y};
			xy_t leftnorm = {camedge1.x - transp1.x, camedge1.y - transp1.y};
			xy_t rightnorm = {camedge2.x - transp1.x, camedge2.y - transp1.y};

			double leftuv = vectorProjectScalar(leftnorm, norm) / edge->uvdiv;
			double rightuv = vectorProjectScalar(rightnorm, norm) / edge->uvdiv;
			renderWall(texture, textures + edge->texture, sector, cam, edge, camedge1, camedge2, leftuv, rightuv);
		}
	}

	// Render the sprites
	sprite_t *sprite = (sprite_t*)sector->lastsprite;
	while(sprite != NULL){
		xy_t relp = {cam->pos.x - sprite->pos.x, cam->pos.z - sprite->pos.z};
		xy_t transp = {.y = anglesin * relp.x + anglecos * relp.y};
		if(transp.y <= cam->znear){
			goto next_sprite;
		}
		transp.x = anglecos * relp.x - anglesin * relp.y;

		if(vectorIsLeft(transp, XY_ZERO, camleftnorm) || !vectorIsLeft(transp, XY_ZERO, camrightnorm)){
			goto next_sprite;
		}

		renderSprite(texture, textures + sprite->texture, cam, sprite, transp);

next_sprite:
		sprite = sprite->prev;
	}
}

void calculateViewport(camera_t *cam, xy_t right)
{
	xy_t camunit = vectorUnit(right);
	cam->fov = camunit.x * camunit.y * 2;
}

void renderFromSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam)
{
	double camdis = cam->zfar - cam->znear;
	xy_t camleft = {camdis * -cam->fov, camdis};
	xy_t camright = {camdis * cam->fov, camdis};

	renderSector(texture, textures, sector, cam, camleft, camright, NULL);
}
