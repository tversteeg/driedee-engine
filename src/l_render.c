#include "l_render.h"

#include <math.h>

#include "l_vector.h"

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

void renderWall(texture_t *target, texture_t *wall, camera_t *cam, edge_t *edge, xy_t left, xy_t right, double leftuv, double rightuv)
{
	// Find x position on the near plane
	//TODO change near plane from 1 to cam->znear
	double projleftx = (left.x / left.y) * cam->fov;
	double projrightx = (right.x / right.y) * cam->fov;

	int halfwidth = target->width >> 1;

	// Convert to screen coordinates
	int screenleftx = halfwidth + projleftx * halfwidth;
	int screenrightx = halfwidth + projrightx * halfwidth;
	if(screenleftx == screenrightx){
		return;
	}

	// Divide by the z value to get the distance and calculate the height with that
	int top = 20;
	int bot = -5;

	double projtoplefty = (top + cam->pos.y) / left.y;
	double projbotlefty = (bot + cam->pos.y) / left.y;
	double projtoprighty = (top + cam->pos.y) / right.y;
	double projbotrighty = (bot + cam->pos.y) / right.y;

	int screentoplefty = halfwidth - projtoplefty * halfwidth;
	int screenbotlefty = halfwidth - projbotlefty * halfwidth;
	int screentoprighty = halfwidth - projtoprighty * halfwidth;
	int screenbotrighty = halfwidth - projbotrighty * halfwidth;

	int screenwidth = screenrightx - screenleftx;
	double slopetop = (screentoprighty - screentoplefty) / (double)screenwidth;
	double slopebot = (screenbotrighty - screenbotlefty) / (double)screenwidth;
	double uvdiff = (rightuv - leftuv) / screenwidth;

	int x;
	for(x = 0; x < screenwidth; x++){
		int top = screentoplefty + x * slopetop;
		int bot = screenbotlefty + x * slopebot;
		//drawTextureSlice(target, wall, screenleftx + x, top, bot - top, leftuv + (x * uvdiff));
		xy_t v1 = {screenleftx + (double)x, (double)top};	
		xy_t v2 = {v1.x, (double)bot};	
		drawLine(target, v1, v2, (pixel_t){(unsigned char)(leftuv + (x * uvdiff) * 255), 0, 0, 255});
	}
}

static void renderSector(texture_t *texture, texture_t *wall, sector_t *sector, camera_t *cam, xy_t camleft, xy_t camright, edge_t *previous)
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

#ifdef DEBUG_DRAW
		xy_t map1 = {transp1.x + texture->width / 2, texture->height / 2 - transp1.y};
		xy_t map2 = {transp2.x + texture->width / 2, texture->height / 2 - transp2.y};
		drawLine(texture, map1, map2, (pixel_t){255, 255, 255, 255});
#endif

		xy_t unit1 = vectorUnit(transp1);
		xy_t unit2 = vectorUnit(transp2);
		bool isnotinview1 = !vectorIsBetween(unit1, camleftnorm, camrightnorm);
		bool isnotinview2 = !vectorIsBetween(unit2, camleftnorm, camrightnorm);
		if(isnotinview1 && isnotinview2){
			// Clip the edge when it lies next to the camera
			if(vectorIsLeft(transp1, XY_ZERO, camleftnorm) &&
					vectorIsLeft(transp2, XY_ZERO, camleftnorm)){
#ifdef DEBUG_DRAW
				drawLine(texture, map1, map2, (pixel_t){255, 0, 255, 255});
#endif
				continue;
			}else if(!vectorIsLeft(transp1, XY_ZERO, camrightnorm) &&
					!vectorIsLeft(transp2, XY_ZERO, camrightnorm)){
#ifdef DEBUG_DRAW
				drawLine(texture, map1, map2, (pixel_t){0, 255, 0, 255});
#endif
				continue;
			}else if(transp1.y - ((transp2.y - transp1.y) / (transp2.x - transp1.x)) * transp1.x < V_ERROR){
				// Clip the edge when the line 'y = ax + b' is above the camera
#ifdef DEBUG_DRAW
				drawLine(texture, map1, map2, (pixel_t){128, 128, 128, 255});
#endif
				continue;
			}
		}
#ifdef DEBUG_DRAW
		drawLine(texture, map1, map2, (pixel_t){0, 0, 255, 255});
#endif

		xy_t camedge1 = transp1;
		if(isnotinview1 && !vectorIsEqual(transp1, camleft) && !vectorIsEqual(transp1, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &camedge1, transp2);
		}
		xy_t camedge2 = transp2;
		if(isnotinview2 && !vectorIsEqual(transp2, camleft) && !vectorIsEqual(transp2, camright)){
			clipPointToCamera(camleftnorm, camrightnorm, &camedge2, transp1);
		}

#ifdef DEBUG_DRAW
		map1 = (xy_t){camedge1.x + texture->width / 2, texture->height / 2 - camedge1.y};
		map2 = (xy_t){camedge2.x + texture->width / 2, texture->height / 2 - camedge2.y};
		drawLine(texture, map1, map2, (pixel_t){255, 0, 0, 255});
#endif

		double cross = vectorCrossProduct(camedge1, camedge2);
		if(cross > 0){
			xy_t temp = transp1;
			transp1 = transp2;
			transp2 = temp;

			temp = camedge1;
			camedge1 = camedge2;
			camedge2 = temp;
		}

		edge_t *neighbor = edge->neighbor;
		if(edge->type == PORTAL && neighbor != NULL){
			renderSector(texture, wall, neighbor->sector, cam, camedge1, camedge2, neighbor);

#ifdef DEBUG_DRAW
			xy_t mapline1;
			mapline1.x = texture->width / 2 + camedge1.x;
			mapline1.y = texture->height / 2 - camedge1.y;

			xy_t normal1 = mapline1;
			normal1.x += camleftnorm.x * 100;
			normal1.y -= camleftnorm.y * 100;
			
			drawLine(texture, mapline1, normal1, (pixel_t){64, 64, 255, 255});
		
			xy_t mapline2;
			mapline2.x = texture->width / 2 + camedge2.x;
			mapline2.y = texture->height / 2 - camedge2.y;

			xy_t normal2 = mapline2;
			normal2.x += camrightnorm.x * 100;
			normal2.y -= camrightnorm.y * 100;

			drawLine(texture, mapline2, normal2, (pixel_t){255, 64, 64, 255});
#endif
		}else if(edge->type == WALL){
			xy_t norm = {transp2.x - transp1.x, transp2.y - transp1.y};
			xy_t leftnorm = {camedge1.x - transp1.x, camedge1.y - transp1.y};
			xy_t rightnorm = {camedge2.x - transp1.x, camedge2.y - transp1.y};

			double leftuv = vectorProjectScalar(leftnorm, norm) / edge->uvdiv;
			double rightuv = vectorProjectScalar(rightnorm, norm) / edge->uvdiv;
			renderWall(texture, wall, cam, edge, camedge1, camedge2, leftuv, rightuv);
		}
	}
}

void calculateViewport(camera_t *cam, xy_t right)
{
	xy_t camunit = vectorUnit(right);
	cam->fov = camunit.x * camunit.y * 2;
}

void renderFromSector(texture_t *texture, texture_t *wall, sector_t *sector, camera_t *cam)
{
	double camdis = cam->zfar - cam->znear;
	xy_t camleft = {camdis * -cam->fov, camdis};
	xy_t camright = {camdis * cam->fov, camdis};

	renderSector(texture, wall, sector, cam, camleft, camright, NULL);
}
