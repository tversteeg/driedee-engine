#include "l_render.h"

#include "l_vector.h"
#include "l_level.h"
#include "l_colors.h"

#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static v_t *screenangles = NULL;
static unsigned int screenw = 0;
static unsigned int screenhw = 0;
static unsigned int screenh = 0;

static void clipPointToCamera(xy_t camleft, xy_t camright, xy_t *p1, xy_t p2)
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

static void renderWall(texture_t *target, const texture_t *tex, const sector_t *sect, edge_t *edge, int left, int right)
{
	unsigned int i;
	for(i = left + screenhw; i < right + screenhw - 1; i++){
		drawLine(target, (xy_t){i, 0}, (xy_t){i, screenh}, COLOR_RED);
	}
}

#if 0
static void renderWall(texture_t *target, const texture_t *tex, const sector_t *sect, const camera_t *cam, edge_t *edge, xy_t left, xy_t right, v_t leftuv, v_t rightuv)
{
	// Find x position on the near plane
	//TODO change near plane from 1 to cam->znear
	v_t projleftx = (left.x / left.y);// * cam->fov;
	v_t projrightx = (right.x / right.y);// * cam->fov;

	int halfwidth = target->width >> 1;
	int halfheight = target->height >> 1;

	// Convert to screen coordinates
	int screenleftx = halfwidth + (projleftx * halfwidth);
	int screenrightx = halfwidth + (projrightx * halfwidth);
	if(screenleftx == screenrightx){
		return;
	}

	v_t ceilheight = sect->ceil + cam->pos.y;
	v_t floorheight = sect->floor + cam->pos.y;

	// Divide by the z value to get the distance and calculate the height with that
	v_t projtoplefty = ceilheight / left.y;
	v_t projbotlefty = floorheight / left.y;
	v_t projtoprighty = ceilheight / right.y;
	v_t projbotrighty = floorheight / right.y;

	int screentoplefty = halfheight - projtoplefty * halfheight;
	int screenbotlefty = halfheight - projbotlefty * halfheight;
	int screentoprighty = halfheight - projtoprighty * halfheight;
	int screenbotrighty = halfheight - projbotrighty * halfheight;

	int screenwidth = screenrightx - screenleftx;
	v_t slopetop = (screentoprighty - screentoplefty) / (v_t)screenwidth;
	v_t slopebot = (screenbotrighty - screenbotlefty) / (v_t)screenwidth;

	const texture_t *walltex = tex + edge->texture;
	const texture_t *ceiltex = tex + sect->ceiltex;
	const texture_t *floortex = tex + sect->floortex;

	v_t anglesin = sin(-cam->angle - M_PI / 2.0);
	v_t anglecos = cos(-cam->angle - M_PI / 2.0);

	int x;
	for(x = 0; x < screenwidth; x++){
		int screenx = screenleftx + x;
		int top = screentoplefty + x * slopetop;
		int bot = screenbotlefty + x * slopebot;
		// Affine transformation
		/*
			 v_t uvdiff = (rightuv - leftuv) / (v_t)screenwidth;
			 drawTextureSlice(target, textures, screenleftx + x, top, bot - top, leftuv + (x * uvdiff));
			 */

		// Perspective transformation
		/* Naive method
			 v_t alpha = x / (v_t)screenwidth;
			 v_t uvx = ((1 - alpha) * (leftuv / left.y) + alpha * (rightuv / right.y)) / ((1 - alpha) / left.y + alpha / right.y);
			 */
		v_t xt1 = (screenwidth - x) * right.y;
		v_t xt2 = x * left.y;
		v_t uvx = (leftuv * xt1 + rightuv * xt2) / (xt1 + xt2);
		drawTextureSlice(target, walltex, screenx, top, bot - top, uvx);

		// Draw ceiling
		if(top > 0){
			int y;
			for(y = 0; y < top; y++){
				v_t relscreeny = (halfheight * ceilheight) / (v_t)(halfheight - y);
				v_t relscreenx = ((screenx - halfwidth) / (v_t)halfwidth) * relscreeny;

				v_t mapx = cam->pos.x + anglecos * relscreeny + anglesin * relscreenx;
				v_t mapy = cam->pos.z + anglesin * relscreeny - anglecos * relscreenx;

				pixel_t pixel = ceiltex->pixels[((int)mapx % ceiltex->width) + ((int)mapy % ceiltex->height) * ceiltex->width];
				setPixel(target, screenx, y, pixel);
/*
				// Draw depth buffer
				uint32_t depthpixel = relscreeny * cam->zfar;	
				memcpy(&pixel, &depthpixel, 4);
				setPixel(target + 1, screenx, y, pixel);
*/
			}
		}
		// Draw floor
		if(bot < (int)target->height){
			int y;
			for(y = bot; y < (int)target->height; y++){
				v_t relscreeny = (halfheight * floorheight) / (v_t)(halfheight - y);
				v_t relscreenx = ((screenx - halfwidth) / (v_t)halfwidth) * relscreeny;

				v_t mapx = cam->pos.x + anglecos * relscreeny + anglesin * relscreenx;
				v_t mapy = cam->pos.z + anglesin * relscreeny - anglecos * relscreenx;

				pixel_t pixel = floortex->pixels[((int)mapx % floortex->width) + ((int)mapy % floortex->height) * floortex->width];
				setPixel(target, screenx, y, pixel);
/*
				// Draw depth buffer
				uint32_t depthpixel = relscreeny * cam->zfar;	
				memcpy(&pixel, &depthpixel, 4);
				setPixel(target + 1, screenx, y, pixel);
*/
			}
		}
	}
}
#endif

static void renderSprite(texture_t *target, const texture_t *sheet, const camera_t *cam, sprite_t *sprite, xy_t pos)
{
	xy_t proj = {(pos.x / pos.y) * cam->fov, (sprite->pos.y + cam->pos.y) / pos.y};

	int halfwidth = target->width >> 1;
	int halfheight = target->height >> 1;

	xy_t scale = {sprite->scale.x * (1.0 / pos.y), sprite->scale.y * (1.0 / pos.y)};

	int screenx = halfwidth + proj.x * halfwidth - (sheet->width >> 1) * scale.x;
	int screeny = halfheight - proj.y * halfheight - sheet->height * scale.y;

	drawTextureScaled(target, sheet, screenx, screeny, scale);
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

		// Clip everything behind the camera
		if(transp1.y <= 0 && transp2.y <= 0){
			continue;
		}

		//TODO fix rounding error here
		transp1.x = acos * relp1.x - asin * relp1.y;
		transp2.x = acos * relp2.x - asin * relp2.y;

		v_t angle1 = (v_t)atan2(transp1.y, transp1.x);
		v_t angle2 = (v_t)atan2(transp2.y, transp2.x);

		bool isleftcam1 = angle1 < screenangles[camleft];
		bool isrightcam1 = angle1 > screenangles[camright];
		bool isleftcam2 = angle2 < screenangles[camleft];
		bool isrightcam2 = angle2 > screenangles[camright];

		bool isnotinview1 = isleftcam1 && isrightcam1;
		bool isnotinview2 = isleftcam2 && isrightcam2;
		if(isnotinview1 && isnotinview2 ){
			// Clip the edge when it lies next to the camera
			if((isleftcam1 && isleftcam2) || (isrightcam1 && isrightcam2)){
				continue;
			}else if(transp1.y - ((transp2.y - transp1.y) / (transp2.x - transp1.x)) * transp1.x < V_ERROR){
				// Clip the edge when the line 'y = ax + b' is above the camera
				continue;
			}
		}

		v_t angleleft = min(angle1, angle2);
		v_t angleright = max(angle1, angle2);

		int newleft = cos(angleleft) * screenhw;
		if(newleft < camleft){
			newleft = camleft;
		}

		int newright = cos(angleright) * screenhw;
		if(newright > camright){
			newright = camright;
		}
		/*
		xy_t camedge1 = transp1;
		if(isnotinview1){
			clipPointToCamera(camleftnorm, camrightnorm, &camedge1, transp2);
		}
		xy_t camedge2 = transp2;
		if(isnotinview2){
			clipPointToCamera(camleftnorm, camrightnorm, &camedge2, transp1);
		}
		*/

		if(edge->type == PORTAL){
			edge_t *neighbor = edge->neighbor;
			if(neighbor != NULL){
				//renderSector(texture, textures, getSector(neighbor->sector), cam, newleft, newright, neighbor);
			}
		}else if(edge->type == WALL){
			/*
			xy_t norm = {transp2.x - transp1.x, transp2.y - transp1.y};
			xy_t leftnorm = {camedge1.x - transp1.x, camedge1.y - transp1.y};
			xy_t rightnorm = {camedge2.x - transp1.x, camedge2.y - transp1.y};

			v_t leftuv = vectorProjectScalar(leftnorm, norm) / edge->uvdiv;
			v_t rightuv = vectorProjectScalar(rightnorm, norm) / edge->uvdiv;

			renderWall(texture, textures, sector, cam, edge, camedge1, camedge2, leftuv, rightuv);
			*/
			renderWall(texture, textures, sector, edge, newleft, newright);
		}
	}
/*
	// Render the sprites
	sprite_t *sprite = (sprite_t*)sector->lastsprite;
	while(sprite != NULL){
		xy_t relp = {cam->pos.x - sprite->pos.x, cam->pos.z - sprite->pos.z};
		xy_t transp = {.y = asin * relp.x + acos * relp.y};
		if(transp.y <= cam->znear){
			goto next_sprite;
		}
		transp.x = acos * relp.x - asin * relp.y;

		if(vectorIsLeft(transp, XY_ZERO, camleftnorm) || !vectorIsLeft(transp, XY_ZERO, camrightnorm)){
			goto next_sprite;
		}

		renderSprite(texture, textures + sprite->texture, cam, sprite, transp);

next_sprite:
		sprite = sprite->prev;
	}
	*/
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
	cam->fov = camunit.x * camunit.y;
}

void initRender(unsigned int width, unsigned int height, camera_t *cam)
{
	screenw = width;
	screenh = height;
	screenhw = width / 2;

	screenangles = (v_t*)malloc(sizeof(v_t) * screenw);
	unsigned int i;
	for(i = 0; i < screenw; i++){
		screenangles[i] = (v_t)atan2(1, (i - screenhw) * cam->fov);
	}
}

void renderFromSector(texture_t *texture, texture_t *textures, sector_t *sector, camera_t *cam)
{
	renderSector(texture, textures, sector, cam, -(int)screenhw, (int)screenhw, NULL);
}
